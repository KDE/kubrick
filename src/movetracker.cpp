/*******************************************************************************
  Copyright 2008 Ian Wadham <iandw.au@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "movetracker.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

MoveTracker::MoveTracker (QWidget * parent)
    :
    QObject (parent),
    myParent (parent)
{
    init();
    rotationState.quaternionSetIdentity();
    rotationState.quaternionToMatrix (rotationMatrix);
}


MoveTracker::~MoveTracker()
{
}


void MoveTracker::init()
{
    currentButton = Qt::NoButton;	// No mouse button being pressed.
    clickFace1    = false;		// No face clicked by mouse button.
    foundFace1    = false;		// Starting face of move not found.
    foundFace2    = false;		// Finishing face of move not found.
    foundHandle   = false;		// Rotation-handle not found.
    moveAngle     = 0;			// No slice-move to be made (yet).
}


void MoveTracker::mouseInput (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int button, int mX, int mY)
{
    if (event == ButtonDown) {
	init();
	currentButton = button;
    }

    if (currentButton == Qt::RightButton) {
	// Right-button is down: rotate the whole cube.
	trackCubeRotation (sceneID, cubeViews, event, mX, mY);
    }
    else if (currentButton == Qt::LeftButton) {
	// Left-button is down: move a slice of the cube.
	trackSliceMove (sceneID, cubeViews, cube, event, mX, mY);
    }

    if (event == ButtonUp) {
	currentButton = Qt::NoButton;
    }
}


void MoveTracker::trackCubeRotation (int sceneID, QList<CubeView *> cubeViews,
		MouseEvent event, int mX, int mY)
{
    if (foundHandle) {
	// Move the handle-point to a new position while rotating the cube
	// around its central point.  The mouse-pointer appears to be attached
	// to the handle-point as the cube rotates.

	if ((mX == mX1) && (mY == mY1)) {
	    return;		// No change in position of mouse.
	}
	mX1 = mX;
	mY1 = mY;

	double axis [nAxes] = {1.0, 0.0, 0.0};
	double degrees = 0.0;

	// Calculate the angle and axis of rotation.

	// If the angle was effectively zero, avoid further calculation.
	if (calculateRotation (mX, mY, axis, degrees)) {
	    // Else, add the rotation to the quaternion and the OpenGL matrix.
	    rotationState.quaternionAddRotation (axis, degrees);
	    rotationState.quaternionToMatrix (rotationMatrix);
	    emit cubeRotated ();
	}
    }
    else if (event != ButtonUp) {
	// Look for a handle-point: a point on the surface of a cube that can
	// be used to rotate the cube around its central point.  Any point will
	// do, provided it is visible and on the surface of a cube.

	GLfloat depth;
	double position [nAxes];

	// Get the mouse position in OpenGL world co-ordinates.
	depth = getMousePosition (mX, mY, position);

	// Find which picture of a cube the mouse is on.
	int cubeID = findWhichCube (sceneID, cubeViews, position);
	if (cubeID < 0) {
	    // Could not find the nearest cube (should never happen).
	    return;
	}
	v = cubeViews.at (cubeID);

	if (position[Z] > (-maxZ + 0.1)) {
	    // Get the mouse position relative to the centre of the cube found.
	    // Use the transformation matrix for the translated and standardly
	    // aligned view of that cube, without including any user's rotation.

	    getGLPosition (mX, mY, depth, v->matrix0, handle);
	    RR = handle[X]*handle[X] + handle[Y]*handle[Y] +
					handle[Z]*handle[Z];
	    R = sqrt (RR);
	    foundHandle = true;
	    mX1 = mX;
	    mY1 = mY;
	}
    }
}


bool MoveTracker::calculateRotation (const int mX, const int mY,
					double axis[], double & degrees)
{
    bool result = true;

    // Get two points on the line of sight, in the nearest cube's co-ordinates.
    // Use the transformation matrix for the translated and standardly aligned
    // view of that cube, without including any user's rotation.

    double p1 [nAxes];
    double p2 [nAxes];

    // The "depth" in OpenGL is a normalised number in the range 0.0 to 1.0.
    // We choose values 0.5 and 1.0 (background) for the two depths.

    getGLPosition (mX, mY, 0.5, v->matrix0, p1);
    getGLPosition (mX, mY, 1.0, v->matrix0, p2);

    // Find where the line of sight intersects the sphere containing the handle.
    // To do this, we use the parametrised equation of a line between two
    // points, i.e.  p = p1 + lambda * (p2 - p1) for any point p.  At those two
    // points, px*px + py*py + pz*pz = R*R, which gives us a quadratic equation
    // to solve for lambda: a*lambda*lambda + b*lambda +c = 0.  So we calculate
    // the coefficents a, b, and c.

    double dx = (p2[X] - p1[X]);
    double dy = (p2[Y] - p1[Y]);
    double dz = (p2[Z] - p1[Z]);
    double a  = dx*dx + dy*dy + dz*dz;
    double b  = 2.0*(p1[X]*dx + p1[Y]*dy + p1[Z]*dz);
    double c  = p1[X]*p1[X] + p1[Y]*p1[Y] + p1[Z]*p1[Z] - RR;

    // Apply the quadratic formula to solve for the nearest of the two lambdas.
    double q  = b*b - 4.0*a*c;
    double lambda = 0.0;
    if (q >= 0.0) {
	lambda = (-b - sqrt (q)) / (2.0*a);
    }
    else {
	// The line of sight to the mouse pointer is outside the handle-sphere.
	// The sphere must not turn any more, otherwise it flips unpredictably.
	degrees = 0.0;
	axis [X] = 1.0;
	axis [Y] = 0.0;
	axis [Z] = 0.0;
	return false;
    }

    // Set up a vector for the old position on the handle-sphere.
    double v1 [nAxes];

    v1 [X] = handle [X];
    v1 [Y] = handle [Y];
    v1 [Z] = handle [Z];

    // Set the new handle position on the handle-sphere.
    handle [X] = (p1[X] + lambda*dx);
    handle [Y] = (p1[Y] + lambda*dy);
    handle [Z] = (p1[Z] + lambda*dz);

    result = getTurnVector (v1, handle, axis, degrees);
    return result;
}


void MoveTracker::trackSliceMove (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int mX, int mY)
{
    double position [nAxes];

    if ((foundHandle) && ((mX != mX1) || (mY != mY1))) {
	mX1 = mX;
	mY1 = mY;

	// Change the move axis and direction only when the mouse pointer moves
	// away from the handle area or after it returns to it and moves away
	// again.  This prevents rapid switching and oscillation of the slices
	// when the pointer is in a diagonal direction from the handle-point.

	// IDW bool outsideHandleArea = (abs (mX - mX0) > 15) || (abs (mY - mY0) > 15);
	bool outsideHandleArea = ((mX - mX0)*(mX - mX0) + (mY - mY0)*(mY - mY0))
				> 400;

	if ((moveAngle == 0) && outsideHandleArea) {
	    // Mouse just moved outside handle area: find the required move.
	    Axis   direction = X;
	    double distance = 0.0;

	    // Get two points on line of sight, in the nearest cube's co-ords.
	    // Use the transformation matrix for the fully translated and
	    // rotated view of that cube, including any user's rotations.

	    double point1 [nAxes];
	    double point2 [nAxes];

	    // The "depth" in OpenGL is a normalised number in the range 0.0 to
	    // 1.0, so use values 0.5 and 1.0 (background) for the two depths.

	    getGLPosition (mX, mY, 0.5, v->matrix, point1);
	    getGLPosition (mX, mY, 1.0, v->matrix, point2);

	    // Find where the line of sight intersects the plane of the found
	    // face.  To do this, use the parametrised equation of a line
	    // between two points: p = p2 + lambda * (p1 - p2) for any point p.

	    double plane  = handle[noTurn];
	    double lambda = (plane - point2[noTurn]) /
                                (point1[noTurn] - point2[noTurn]);
	    LOOP (n, nAxes) {
		if (n == noTurn) {
		    position[n] = plane;
		}
		else {
		    position[n] = point2[n] + lambda * (point1[n] - point2[n]);
		}
	    }

	    // Find in which direction the mouse moved most.
	    LOOP (n, nAxes) {
		if (n != noTurn) {
		    double d = position[n] - handle[n];
		    if (fabs (d) > fabs (distance)) {
			distance = d;
			direction = (Axis) n;
		    }
		}
	    }

	    // Turn axis will be at right angles to mouse move and face-normal.
	    currentMoveAxis = (((direction + 1) % nAxes) == noTurn) ?
				(Axis) ((direction + 2) % nAxes) :
				(Axis) ((direction + 1) % nAxes);

	    // Set up unit vectors for the mouse move and the face-normal.
	    int moveVec[nAxes] = {0, 0, 0};
	    int faceVec[nAxes] = {0, 0, 0};
	    moveVec[direction] = (distance < 0.0) ? -1 : +1;
	    faceVec[noTurn]    = (face1[noTurn] < 0.0) ? -1 : +1;

	    // Form a partial cross-product to get the direction of the turn.
	    // The other components of turnVec must be zero (3 orthogonal axes).

	    int a = (currentMoveAxis + 1) % nAxes;	// Get non-turn axes in
	    int b = (currentMoveAxis + 2) % nAxes;	// cyclical order.
	    int turnVec = moveVec[a]*faceVec[b] - moveVec[b]*faceVec[a];

	    currentMoveDirection = (turnVec < 0) ? ANTICLOCKWISE : CLOCKWISE;
	    moveAngle            = (turnVec < 0) ? -6 : 6;

	    currentMoveSlice     = face1[currentMoveAxis];
	    cube->setMoveInProgress (currentMoveAxis, currentMoveSlice);
	}
	else if (! outsideHandleArea) {
	    moveAngle = 0;
	}

	cube->setMoveAngle (moveAngle);	// If zero, no move will be triggered.
    }

    // Start by looking for a handle-point from which to make a slice move.
    else if ((! foundHandle) && (event != ButtonUp)) {
	// Get the mouse position in OpenGL world co-ordinates.
	GLfloat depth;
	depth = getMousePosition (mX, mY, position);

	// Continue only if the mouse hit a cube, not the background.
	if (position[Z] > (-maxZ + 0.1)) {

	    // Find which picture of a cube the mouse is on.
	    int cubeID = findWhichCube (sceneID, cubeViews, position);
	    if (cubeID < 0) {
		// Could not find the nearest cube (should never happen).
		return;
	    }
	    v = cubeViews.at (cubeID);

	    // Get the mouse position relative to the centre of the cube found.
	    // Use the transformation matrix for the fully translated and
	    // rotated view of that cube, including any user's rotations.

	    getGLPosition (mX, mY, depth, v->matrix, handle);

	    // Find the sticker at that (handle) position.
	    if (! cube->findSticker (handle, v->cubieSize, face1)) {
		// Could not find the sticker (should never happen).
		return;
	    }

	    // Find the axis that is NOT a possible axis of the slice move.
	    noTurn = cube->faceNormal (face1);
	    foundHandle = true;

	    // Save the mouse-position for future tracking and comparison.
	    mX0 = mX; mY0 = mY;
	    mX1 = mX; mY1 = mY;
	}
    }

    // After a button-release, perform the slice move required (if any).
    if (event == ButtonUp) {
	if (moveAngle != 0) {
	    // We found a move.
	    Move * move          = new Move;
	    move->axis           = currentMoveAxis;
	    move->slice          = currentMoveSlice;
	    move->direction      = currentMoveDirection;

	    emit newMove (move);	// Signal Game obj to store this move.
	}
	moveAngle = 0;
	cube->setMoveAngle (0);
    }
}


void MoveTracker::realignCube (QList<Move *> & tempMoves)
{
    double fromAxis [nAxes * nAxes] = {1.0, 0.0, 0.0,	// X axis.
                                       0.0, 1.0, 0.0,	// Y axis.
                                       0.0, 0.0, 1.0};	// Z axis.
    double toAxis   [nAxes * nAxes] = {1.0, 0.0, 0.0,	// X axis.
                                       0.0, 1.0, 0.0,	// Y axis.
                                       0.0, 0.0, 1.0};	// Z axis.

    // Find where the original X, Y and Z axes have ended up.
    rotateAxes (rotationState, fromAxis, toAxis);

    // Find which component of which axis is closest to X, Y, Z, -X, -Y or -Z.
    int bestAligned = -1;
    double component = 0.0;
    LOOP (n, nAxes) {
	int k = n * nAxes;
	LOOP (m, nAxes) {
	    // Pick the largest component as the best aligned.
	    if (fabs(toAxis [k + m]) > component) {
		bestAligned = k + m;
		component = fabs(toAxis [k + m]);
	    }
	}
    }

    // Determine which axis (X, -X, etc.) is closest to the best-aligned one.
    int nAxisTo = bestAligned % nAxes;

    double v1 [nAxes] = {0.0, 0.0, 0.0};	// The first axis to be aligned.
    double v2 [nAxes] = {0.0, 0.0, 0.0};	// The axis to align it to.

    int offset = bestAligned - nAxisTo;
    LOOP (n, nAxes) {
	v1 [n] = toAxis [offset + n];
    }

    // Determine whether the alignment will be parallel or anti-parallel.
    v2 [nAxisTo] = toAxis [bestAligned] < 0.0 ? -1.0 : 1.0;

    // Calculate the turn required to align the best-aligned axis exactly.
    double turnVector [nAxes] = {1.0, 0.0, 0.0};
    double turnAngle = 0.0;
    if (getTurnVector (v1, v2, turnVector, turnAngle)) {
	// Apply the required turn to the cube images (if non-zero).
	rotationState.quaternionAddRotation (turnVector, turnAngle);
	rotationState.quaternionToMatrix (rotationMatrix);
    }

    // Find where the original X, Y and Z axes are now.
    rotateAxes (rotationState, fromAxis, toAxis);

    // The remaining two axes must rotate in the plane perpendicular to the axis
    // to which the first axis was aligned.  Once one of the two axes is in
    // place, the other axis will automatically follow.

    double v3 [nAxes] = {0.0, 0.0, 0.0};	// The next axis to be aligned.
    double v4 [nAxes] = {0.0, 0.0, 0.0};	// The axis with which to align.

    // Pick the next axis to be aligned (in cyclical order).
    int nAxis2 = ((bestAligned / nAxes) + 1) % nAxes;
    v3 [nAxis2] = 1.0;

    // Find where that axis is now, after the first alignment.
    rotationState.quaternionRotateVector (v3);

    // Find which axis is the closest axis one to align to.
    component = 0.0;
    nAxisTo = 0;
    LOOP (n, nAxes) {
	if (fabs (v3 [n]) > component) {
	    component = fabs (v3 [n]);
	    nAxisTo = n;
	}
    }

    // Determine whether the alignment will be parallel or anti-parallel.
    v4 [nAxisTo] = v3 [nAxisTo] < 0.0 ? -1.0 : 1.0;

    // Calculate the turn required to align the two remaining axes exactly.
    if (getTurnVector (v3, v4, turnVector, turnAngle)) {
	// Apply the required turn to the cube images (if non-zero).
	rotationState.quaternionAddRotation (turnVector, turnAngle);
	rotationState.quaternionToMatrix (rotationMatrix);
    }

    // The original X, Y and Z axes should now be aligned, probably with other
    // axes.  So find out what is aligned with what and make the 90 or 180
    // degree moves of the underlying cube needed to get to that position.

    rotateAxes (rotationState, fromAxis, toAxis);
    makeWholeCubeMoveList (tempMoves, toAxis);

    // Finally set the cube images to the unrotated state.
    rotationState.quaternionSetIdentity();
    rotationState.quaternionToMatrix (rotationMatrix);
}


void MoveTracker::makeWholeCubeMoveList (QList<Move *> & tempMoves,
			const double to [nAxes * nAxes])
{
    // The cube has been rotated by the player and aligned to the standard
    // orientation (top, front and right faces visible), however it will
    // probably not be in its original orientation.  This procedure works
    // out a series of 90 and 180 degree moves that will get back there.
    // The reverse of these moves is added to the player's list of moves
    // and applied to the internal Cube object, so that the player's manual
    // rotations are replaced by the equivalent 90 and 180 degree Cube moves.
    //
    // This makes keyboard and Singmaster-notation moves once again meaningful.
    // For example, the Y-axis is again the one pointing up and T (top) again
    // represents the top face of the cube, as seen by the player.

    int  toI   [nAxes] [nAxes] = {{0, 0, 0},
                                  {0, 0, 0},
                                  {0, 0, 0}};
    bool notAligned = true;
    int  safetyLimit = 3;

    LOOP (row, nAxes) {
	LOOP (col, nAxes) {
	    if (fabs (to [row * nAxes + col]) > 0.999) {
		toI [row] [col] = (to [row * nAxes + col] < 0.0) ? -1 : +1;
	    }
	}
    }

    while (notAligned) {
	if (--safetyLimit <= 0) notAligned = false;
	// Determine whether any axes are fully aligned.
	if ((toI [X][X] == 1) || (toI [Y][Y] == 1) || (toI [Z][Z] == 1)) {
	    // At least one axis is fully aligned.
	    if ((toI [X][X] == 1) && (toI [Y][Y] == 1) && (toI [Z][Z] == 1)) {
		// All axes are fully aligned: time to stop.
		notAligned = false;
		break;
	    }
	    // Find which axis is fully aligned and what move aligns the others.
	    else {
		int aligned  = -1;
		int reversed = -1;
		LOOP (n, nAxes) {
		    if (toI [n][n] == -1) {
			// This axis is reversed.
			reversed = n;
		    }
		    if (toI [n][n] == 1) {
			// This axis is fully aligned.
			aligned = n;
		    }
		}
		if (aligned < 0) {
		    // Should never happen ...
		}
		// Calculate whether to rotate 90, -90 or 180 degrees.
		Axis a = (Axis) aligned;
		if (reversed >= 0) {
		    // One axis is aligned and both of the others are reversed.
		    // A single 180 degree move should bring all axes into line.

		    prepareWholeCubeMove (tempMoves, toI, a, ONE_EIGHTY);
		}
		else {
		    // A single 90 degree move should bring all axes into line.

		    // Calculate the required rotation around axis "a" from the
		    // positions of the other two axes (n1 and n2).  For example
		    // if a is the Y axis, then n1 is the Z axis and n2 is the
		    // X axis (in cyclical order), so the Y axis (n1) is either
		    // pointing to +Z or -Z, as represented by (toI [Y][Z]).

		    Axis     n1 = (Axis) ((a + 1) % nAxes);
		    Axis     n2 = (Axis) ((a + 2) % nAxes);
		    prepareWholeCubeMove (tempMoves, toI, a,
			    (toI [n1][n2] > 0) ? CLOCKWISE : ANTICLOCKWISE);
		}
	    }
	}
	// No axes are fully aligned: pick one to align.
	else {
	    int reversed = -1;
	    LOOP (n, nAxes) {
		if (toI [n][n] == -1) {
		    // This axis is reversed.
		    reversed = n;
		    break;
		}
	    }
	    if (reversed >= 0) {
		// Pick an axis that is not reversed and rotate it 180 degrees.
		// That should bring one axis into line.

		prepareWholeCubeMove (tempMoves, toI,
			(Axis) ((reversed + 1) % nAxes), ONE_EIGHTY);
	    }
	    else {
		// Pick any axis and rotate it 90 or -90 degrees to align it.
		// If X is parallel/anti-parallel to the Z axis, we need to
		// rotate around the Y axis and vice-versa.
		Axis     rAxis = (toI [X][Y] == 0) ? Y : Z;
		Rotation r     = CLOCKWISE;
		r = (((rAxis == Y) && (toI [X][Z] > 0)) ||	// If X is at +Z
		     ((rAxis == Z) && (toI [X][Y] < 0))) ?	// or -Y, turn
			ANTICLOCKWISE : r;			// anti-clock.
		prepareWholeCubeMove (tempMoves, toI, rAxis, r);
	    }
	}
    }
}


void MoveTracker::prepareWholeCubeMove (QList<Move *> & moveList,
			int to [nAxes][nAxes], const Axis a, const Rotation d)
{
    int dir   = (d == ANTICLOCKWISE) ? -1 : +1;
    int count = (d == ONE_EIGHTY)    ? 2  : 1;

    Axis coord1 = (Axis) ((a + 1) % nAxes);
    Axis coord2 = (Axis) ((a + 2) % nAxes);
    int  temp;

    LOOP (n, count) {
	LOOP (i, nAxes) {
	    temp = to [i][coord1];
	    to [i][coord1] = +dir * to [i][coord2];
	    to [i][coord2] = -dir * temp;
	}
    }

    Move * move          = new Move;
    move->axis           = a;
    move->slice          = WHOLE_CUBE;
    move->direction      = CLOCKWISE;
    move->degrees        = 90;

    // Stack the moves onto the list in reverse order.

    if (d == ONE_EIGHTY) {
	// Use two 90 degree moves (making the equivalent Singmaster cube-moves
	// easier to process later on in undo/redo operations, etc.).

	moveList.prepend (move);

	move             = new Move;		// Allocate new heap space.
	move->axis       = a;
	move->slice      = WHOLE_CUBE;
	move->direction  = CLOCKWISE;
	move->degrees    = 90;

	moveList.prepend (move);
    }
    else {
	// Reverse a 90 degree move.
	move->direction  = (d == CLOCKWISE) ? ANTICLOCKWISE : CLOCKWISE;

	moveList.prepend (move);
    }
}


int MoveTracker::findWhichCube (const int sceneID,
		const QList<CubeView *> cubeViews, const double position[])
{
    // For some reason this function cannot compile with return-type CubeView *.

    double distance = 10000.0;			// Large value.
    double d        = 0.0;
    double dx       = 0.0;
    double dy       = 0.0;
    double dz       = 0.0;
    int    indexV   = -1;

    // Find which cube in the current scene is closest to the given position.
    CubeView * v;
    LOOP (n, cubeViews.size()) {
        v = cubeViews.at (n);
	if (v->sceneID != sceneID) {
	    continue;				// Skip unwanted scene IDs.
	}

	dx = position[X] - v->position[X];
	dy = position[Y] - v->position[Y];
	dz = position[Z] - v->position[Z];
	d  = sqrt (dx*dx + dy*dy + dz*dz);	// Pythagoras.
	if (d < distance) {
	    distance = d;
	    indexV   = n;
	}
    }
    return (indexV);
}


void MoveTracker::rotateAxes (const Quaternion & r,
		const double from [nAxes * nAxes], double to [nAxes * nAxes])
{
    LOOP (n, nAxes) {
	int k = n * nAxes;
	LOOP (m, nAxes) {
	    to [k + m] = from [k + m];		// Copy the input vector.
	}
	r.quaternionRotateVector (&(to [k]));	// Rotate it.
    }
}


bool MoveTracker::getTurnVector (
		const double v1 [nAxes], const double v2 [nAxes],
		double turnAxis [nAxes], double & turnAngle)
{
    double radius;
    double u1 [nAxes], u2 [nAxes];
    bool result = true;

    // Make v1 and v2 into unit vectors.
    radius = sqrt (v1[X]*v1[X] + v1[Y]*v1[Y] + v1[Z]*v1[Z]);
    u1[X] = v1[X] / radius;
    u1[Y] = v1[Y] / radius;
    u1[Z] = v1[Z] / radius;

    radius = sqrt (v2[X]*v2[X] + v2[Y]*v2[Y] + v2[Z]*v2[Z]);
    u2[X] = v2[X] / radius;
    u2[Y] = v2[Y] / radius;
    u2[Z] = v2[Z] / radius;

    // The vector is reversed for anti-clockwise rotations.
    // The angle is positive, for both clockwise and anti-clockwise.
    double rad  = acos (u1[X]*u2[X] + u1[Y]*u2[Y] + u1[Z]*u2[Z]);
    if (fabs (rad) >=  0.0001) {
	// The angle is of reasonable size.  It is safe to do the divisions.
	double srad = sin (rad);
	turnAxis[X] = -(u1[Y]*u2[Z] - u1[Z]*u2[Y]) / srad;
	turnAxis[Y] = -(u1[Z]*u2[X] - u1[X]*u2[Z]) / srad;
	turnAxis[Z] = -(u1[X]*u2[Y] - u1[Y]*u2[X]) / srad;
	result = true;			// Calculation succeeded.
    }
    else {
	// The angle is less than 1 minute of arc. Avoid overflow on division.
	rad = 0.0;			// Zero angle (no rotation).
	turnAxis[X] = 1.0;		// Arbitrary axis (does not matter).
	turnAxis[Y] = 0.0;
	turnAxis[Z] = 0.0;
	result = false;			// Calculation failed.
    }
    turnAngle = rad * 180.0 / M_PI;
    radius = sqrt (turnAxis[X]*turnAxis[X] +
		turnAxis[Y]*turnAxis[Y] + turnAxis[Z]*turnAxis[Z]);
    return result;
}


GLfloat MoveTracker::getMousePosition (const int mX, const int mY, double pos[])
{
    GLfloat depth;

    // Read the depth value at the position on the screen.
    glReadPixels (mX, mY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    // Get the mouse position in OpenGL world co-ordinates.
    getAbsGLPosition (mX, mY, depth, pos);

    return depth;
}


void MoveTracker::getAbsGLPosition (int sX, int sY, GLfloat depth, double pos[])
{
    GLdouble m [16];
    glGetDoublev  (GL_MODELVIEW_MATRIX, m);
    getGLPosition (sX, sY, depth, m, pos);
}


void MoveTracker::getGLPosition (int sX, int sY, GLfloat depth,
					double matrix[], double pos[])
{
    // Retrieve the OpenGL projection matrix and viewport.
    GLdouble p [16];
    GLint    v [4];
    glGetDoublev  (GL_PROJECTION_MATRIX, p);
    glGetIntegerv (GL_VIEWPORT, v);

    // Find the world coordinates of the nearest object at the screen position.
    GLdouble objx, objy, objz;
    GLint ret = gluUnProject (sX, sY, depth,
			      matrix, p, v,
			      &objx, &objy, &objz);

    if (ret != GL_TRUE) {
	kDebug() << "gluUnProject() unsuccessful at point" << sX << sY << depth;
	return;
    }

    // Return the OpenGL coordinates we found.
    pos[X] = objx; pos[Y] = objy; pos[Z] = objz;
}


void MoveTracker::usersRotation()
{
    glMultMatrixf (rotationMatrix);
}


