/*******************************************************************************
  Copyright 2008 Ian Wadham <ianw2@optusnet.com.au>

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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <KDebug>
#include <KLocale>
#include <KMessageBox>

#include "movetracker.h"

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
	if ((mX == mX1) && (mY == mY1)) {
	    return;
	}
	mX1 = mX;
	mY1 = mY;

	double axis [nAxes] = {0.0, 0.0, 1.0};
	double degrees = 0.0;

	calculateRotation (mX, mY, axis, degrees);
	rotationState.quaternionAddRotation (axis, degrees);
	rotationState.quaternionToMatrix (rotationMatrix);
    }
    else if (event != ButtonUp) {
	GLfloat depth;
	double position [nAxes];

	// Get the mouse position in OpenGL world co-ordinates.
	depth = getMousePosition (mX, mY, position);
	printf ("GL coords: %7.2f %7.2f %7.2f %7.2f\n",
			position[X], position[Y], position[Z], -maxZ+0.1);

	// If playing, find which picture of a cube the mouse is on.
	int cubeID = findWhichCube (sceneID, cubeViews, position);
	if (cubeID < 0) {
	    // Could not find the nearest cube (should never happen).
	    return;
	}
	v = cubeViews.at (cubeID);

	if (position[Z] > (-maxZ + 0.1)) {
	    LOOP (n, nAxes) {
		handle [n] = position [n] - v->position[n];
		RR = handle[X]*handle[X] + handle[Y]*handle[Y] +
					handle[Z]*handle[Z];
		R = sqrt (RR);
	    }
	    foundHandle = true;
	    mX1 = mX;
	    mY1 = mY;
	    kDebug() << "Found handle:" << handle[0] << handle[1] <<
				handle[2] << "R, RR" << R << RR << mX << mY;
	    if (handle[Z] < 0.0) { // IDW ***
		// If handle[Z] is negative, the first rotation jumps
		// suddenly to positive (60 degrees).  Even with the
		// adjustment below, it can jump 18 degrees ...
		kDebug() << "Reflecting handle[Z] ...";
		handle[Z] = -handle[Z];
	    }
	}
    }
}


void MoveTracker::calculateRotation (const int mX, const int mY,
					double axis[], double & degrees)
{
    bool logging = false;

    // Get two points on the line of sight, in the nearest cube's co-ordinates.
    double p1 [nAxes];
    double p2 [nAxes];

    // The "depth" in OpenGL is a normalised number in the range 0.0 to 1.0.
    // We choose values 0.5 and 1.0 (background) for the two depths.

    getAbsGLPosition (mX, mY, 0.5, p1);
    getAbsGLPosition (mX, mY, 1.0, p2);
    if (logging) {
	printf ("Point1 GL coords:   %7.2f %7.2f %7.2f Mouse at: %d %d\n",
					p1[X], p1[Y], p1[Z], mX, mY);
	printf ("Point2 GL coords:   %7.2f %7.2f %7.2f\n", p2[X], p2[Y], p2[Z]);
    }

    // Get the positions of the points relative to the centre of the cube.
    LOOP (n, nAxes) {
	p1[n] = p1[n] - v->position[n];
	p2[n] = p2[n] - v->position[n];
    }
    if (logging) {
	printf ("Point1 cube coords: %7.2f %7.2f %7.2f\n", p1[X], p1[Y], p1[Z]);
	printf ("Point2 cube coords: %7.2f %7.2f %7.2f\n", p2[X], p2[Y], p2[Z]);
    }

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
    if (logging) {
	printf ("a = %7.2f, b = %7.2f, c = %7.2f, b^2 - 4ac = %7.2f\n",
							a, b, c, q);
    }
    double lambda = 0.0;
    if (q >= 0.0) {
	lambda = (-b - sqrt (q)) / (2.0*a);
    }
    else {
	if (logging) {
	printf ("Line of sight is outside the handle-sphere.\n");
	}
	lambda = -p1[Z] / (p2[Z] - p1[Z]);
    }
    if (logging) {
	printf ("Lambda: %7.2f\n", lambda);
    }

    // Set up unit vectors for the old and new positions on the handle-sphere.
    double u1 [nAxes];
    double u2 [nAxes];

    u1[X] = handle[X] / R;
    u1[Y] = handle[Y] / R;
    u1[Z] = handle[Z] / R;

    u2[X] = (p1[X] + lambda*dx);
    u2[Y] = (p1[Y] + lambda*dy);
    u2[Z] = (p1[Z] + lambda*dz);

    double radius = sqrt (u2[X]*u2[X] + u2[Y]*u2[Y] + u2[Z]*u2[Z]);
    u2[X] = u2[X] / radius;
    u2[Y] = u2[Y] / radius;
    u2[Z] = u2[Z] / radius;
    radius = sqrt (u2[X]*u2[X] + u2[Y]*u2[Y] + u2[Z]*u2[Z]);

    double rad  = acos (u1[X]*u2[X] + u1[Y]*u2[Y] + u1[Z]*u2[Z]);
    double srad = sin (rad);
    axis[X] = -(u1[Y]*u2[Z] - u1[Z]*u2[Y]) / srad;
    axis[Y] = -(u1[Z]*u2[X] - u1[X]*u2[Z]) / srad;
    axis[Z] = -(u1[X]*u2[Y] - u1[Y]*u2[X]) / srad;
    degrees = rad * 180.0 / M_PI;

    LOOP (n, nAxes) {
	handle[n] = u2[n] * R;
    }
    if (logging) {
	printf ("Vector u1: %7.3f, %7.3f, %7.3f\n", u1[X], u1[Y], u1[Z]);
	printf ("Vector u2: %7.3f, %7.3f, %7.3f radius: %7.3f\n",
				u2[X], u2[Y], u2[Z], radius);
	printf ("Angle = %7.2f, axis = %7.2f, %7.2f, %7.2f\n",
				degrees, axis[X], axis[Y], axis[Z]);
	printf ("New handle: %7.2f, %7.2f, %7.2f\n",
				handle[X], handle[Y], handle[Z]);
    }
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
		// IDW printf ("    Point3 [%d] (cube coord): %7.2f\n",
                                // IDW n, (point3 [n] * 2.0) / v->cubieSize);
	    }

	    // Find in which direction the mouse moved most.
	    kDebug() << "Mouse dX:" << (mX - mX0) << "dY:" << (mY - mY0);
	    LOOP (n, nAxes) {
		if (n != noTurn) {
		    double d = position[n] - handle[n];
		    kDebug() << "Axis:" << n << "distance" << d;
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
	    kDebug() << "a,b" << a << b << turnVec;
	    kDebug() << "Direction:" << direction << "distance:" << distance
			<< "axis:" << currentMoveAxis << "angle:" << moveAngle;
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
	printf ("GL coords: %7.2f %7.2f %7.2f %7.2f\n",
			position[X], position[Y], position[Z], -maxZ+0.1);

	// Continue only if the mouse hit a cube, not the background.
	if (position[Z] > (-maxZ + 0.1)) {

	    // Find which picture of a cube the mouse is on.
	    int cubeID = findWhichCube (sceneID, cubeViews, position);
	    if (cubeID < 0) {
		// Could not find the nearest cube (should never happen).
		return;
	    }
	    v = cubeViews.at (cubeID);

	    // Get the mouse position relative to centre of the cube we found.
	    getGLPosition (mX, mY, depth, v->matrix, handle);

	    // Find the sticker at that (handle) position.
	    if (! cube->findSticker (handle, v->cubieSize, face1)) {
		// Could not find the sticker (should never happen).
		return;
	    }

	    // Find the axis that is NOT a possible axis of the slice move.
	    noTurn = cube->faceNormal (face1);
	    kDebug() << "FACE:" << face1[X] << face1[Y] << face1[Z];

	    foundHandle = true;
	    kDebug() << "Found handle:" << handle[0] << handle[1] << handle[2]
			<< mX << mY << "noTurn" << noTurn;

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

	    kDebug() << "Append move:" << currentMoveAxis << currentMoveSlice
					<< currentMoveDirection; // IDW ***
	    emit newMove (move);	// Signal the Game to store this move.
	}
	moveAngle = 0;
	cube->setMoveAngle (0);
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
	kDebug() << "gluUnProject() did not succeed";
	return;
    }

    // Return the OpenGL coordinates we found.
    pos[X] = objx; pos[Y] = objy; pos[Z] = objz;
}


void MoveTracker::usersRotation()
{
    glMultMatrixf (rotationMatrix);
}
