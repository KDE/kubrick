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
    clickFace1 =    false;		// No face clicked by mouse button.
    foundFace1 =    false;		// Starting face of move not found.
    foundFace2 =    false;		// Finishing face of move not found.
    foundHandle =   false;		// Rotation-handle not found.
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
	    kDebug() << "No mouse movement ...";
	    return;
	}
	mX1 = mX;
	mY1 = mY;

	kDebug() << "ROTATING ...";
	double axis [nAxes] = {0.0, 0.0, 1.0};
	double degrees = 0.0;

	calculateRotation (mX, mY, axis, degrees);
	rotationState.quaternionAddRotation (axis, degrees);
	rotationState.quaternionToMatrix (rotationMatrix);
    }
    else if (event != ButtonUp) {
	double position [nAxes];

	// Read the depth value at the position on the screen.
	GLfloat depth;
	glReadPixels (mX, mY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	// If playing, find which picture of a cube the mouse is on.
	// IDW *** CubeView * v = cubeViews.at
	v = cubeViews.at(findWhichCube (sceneID, cubeViews, mX, mY, depth));
	if (v == 0) {
	    // Could not find the nearest cube (should never happen).
	    return;
	}

	// Get the mouse position in OpenGL world co-ordinates.
	getAbsGLPosition (mX, mY, depth, position);
	printf ("GL coords: %7.2f %7.2f %7.2f %7.2f\n",
			position[X], position[Y], position[Z], -maxZ+0.1);

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
		// suddenly to positive (60 degrees).  Even with this
		// adjustment, it can jump 18 degrees ...
		kDebug() << "Reflecting handle[Z] ...";
		handle[Z] = -handle[Z];
	    }
	}
    }
}


void MoveTracker::calculateRotation (const int mX, const int mY,
					double axis[], double & degrees)
{
    // Get two points on the line of sight, in the nearest cube's co-ordinates.
    double p1 [nAxes];
    double p2 [nAxes];

    // The "depth" in OpenGL is a normalised number in the range 0.0 to 1.0.
    // We choose values 0.5 and 1.0 (background) for the two depths.

    getAbsGLPosition (mX, mY, 0.5, p1);
    getAbsGLPosition (mX, mY, 1.0, p2);
    printf ("Point1 GL coords:   %7.2f %7.2f %7.2f Mouse at: %d %d\n",
					p1[X], p1[Y], p1[Z], mX, mY);
    printf ("Point2 GL coords:   %7.2f %7.2f %7.2f\n", p2[X], p2[Y], p2[Z]);
    // Get the positions of the points relative to the centre of the cube.
    LOOP (n, nAxes) {
	p1[n] = p1[n] - v->position[n];
	p2[n] = p2[n] - v->position[n];
    }
    printf ("Point1 cube coords: %7.2f %7.2f %7.2f\n", p1[X], p1[Y], p1[Z]);
    printf ("Point2 cube coords: %7.2f %7.2f %7.2f\n", p2[X], p2[Y], p2[Z]);

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
    printf ("a = %7.2f, b = %7.2f, c = %7.2f, b^2 - 4ac = %7.2f\n", a, b, c, q);
    double lambda = 0.0;
    if (q >= 0.0) {
	lambda = (-b - sqrt (q)) / (2.0*a);
	printf ("Lambda: %7.2f\n", lambda);
    }
    else {
	printf ("Line of sight is outside the handle-sphere.\n");
	lambda = -p1[Z] / (p2[Z] - p1[Z]);
    }

    // Set up unit vectors for the old and new positions on the handle-sphere.
    double u1 [nAxes];
    double u2 [nAxes];

    u1[X] = handle[X] / R;
    u1[Y] = handle[Y] / R;
    u1[Z] = handle[Z] / R;
    printf ("Vector u1: %7.3f, %7.3f, %7.3f\n", u1[X], u1[Y], u1[Z]);

    u2[X] = (p1[X] + lambda*dx);
    u2[Y] = (p1[Y] + lambda*dy);
    u2[Z] = (p1[Z] + lambda*dz);

    double radius = sqrt (u2[X]*u2[X] + u2[Y]*u2[Y] + u2[Z]*u2[Z]);
    u2[X] = u2[X] / radius;
    u2[Y] = u2[Y] / radius;
    u2[Z] = u2[Z] / radius;
    radius = sqrt (u2[X]*u2[X] + u2[Y]*u2[Y] + u2[Z]*u2[Z]);
    printf ("Vector u2: %7.3f, %7.3f, %7.3f radius: %7.3f\n",
				u2[X], u2[Y], u2[Z], radius);

    double rad  = acos (u1[X]*u2[X] + u1[Y]*u2[Y] + u1[Z]*u2[Z]);
    double srad = sin (rad);
    axis[X] = -(u1[Y]*u2[Z] - u1[Z]*u2[Y]) / srad;
    axis[Y] = -(u1[Z]*u2[X] - u1[X]*u2[Z]) / srad;
    axis[Z] = -(u1[X]*u2[Y] - u1[Y]*u2[X]) / srad;
    degrees = rad * 180.0 / M_PI;
    printf ("Angle = %7.2f, axis = %7.2f, %7.2f, %7.2f\n",
				degrees, axis[X], axis[Y], axis[Z]);

    // IDW rotationState.quaternionFromRotation (rotation, axis, angle);
    // IDW rotation.quaternionFromRotation (&rotation, axis, angle);
    LOOP (n, nAxes) {
	handle[n] = u2[n] * R;
    }
    printf ("New handle: %7.2f, %7.2f, %7.2f\n",
				handle[X], handle[Y], handle[Z]);
}


void MoveTracker::trackSliceMove (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int mX, int mY)
{
    bool found = findFaceCentre (sceneID, cubeViews, cube, event, mX, mY);

    // After a button-press, save the cubie face-centre that was found (if any).
    if (event == ButtonDown) {
	if (found) {
	    cube->setMoveAngle (0);
	    // IDW cube->setBlinkingOff ();
	    // IDW blinking = false;
	    // IDW int normal = cube->faceNormal (face1);
	    // IDW LOOP (n, nAxes) {
		// IDW if (n != normal) {
		    // IDW // Show the two slices that might move.
		    // IDW cube->setBlinkingOn ((Axis) n, face1[n]);
		// IDW }
	    // IDW }
	}
    } // End - Button-press event

    if (event == Tracking) {
	// IDW cube->setBlinkingOff ();
	if (evaluateMove (cube) == 1) {
	    // We have identified a single slice: make it blink.
	    // IDW cube->setBlinkingOn (currentMoveAxis, currentMoveSlice);
	    cube->setMoveInProgress (currentMoveAxis, currentMoveSlice);
	    cube->setMoveAngle (currentMoveDirection == CLOCKWISE ? 6 : -6);
	} 
	else if (clickFace1) {
	    // Cannot identify a single slice: make two possible slices blink.
	    cube->setMoveAngle (0);
	    // IDW int normal1 = cube->faceNormal (face1);
	    // IDW LOOP (n, nAxes) {
		// IDW if (n != normal1) {
		    // IDW // Show the two slices that might move.
		    // IDW cube->setBlinkingOn ((Axis) n, face1[n]);
		// IDW }
	    // IDW }
	}
    } // End - Tracking event

    // After a button-release, calculate the slice or whole-cube move required.
    if (event == ButtonUp) {
	int result = evaluateMove (cube);
	// IDW cube->setBlinkingOff ();
	cube->setMoveAngle (0);
	// IDW blinking = false;
	if (result == 1) {
	    // We found a move: located by two different stickers on one slice.
	    Move * move = new Move;
	    move->axis           = currentMoveAxis;
	    move->slice          = currentMoveSlice;
	    move->direction      = currentMoveDirection;

	    kDebug() << "Append move:" << currentMoveAxis << currentMoveSlice
					<< currentMoveDirection; // IDW ***
	    emit newMove (move);	// Signal the Game to store this move.
	}
	// else if (result < 0) {
	    // We found no cubies.
	    // KMessageBox::information (myParent,
		// i18n("You should start or finish with the mouse "
		     // "pointer on a colored sticker, not in the "
		     // "background area."),
		// i18n("Move Error"), "move_error_1");
	// }
	// else if (result == 0) {
	    // The move was skewed between two slices.
	    // KMessageBox::information (myParent,
		// i18n("To turn a slice of the cube you should hold "
		     // "the left mouse button down and drag the pointer "
		     // "from one colored sticker to another on the "
		     // "same slice, or you can go around onto "
		     // "another face of the cube.\n\nIf you try your move "
		     // "again, but slowly, the cube will blink and show "
		     // "you which slices can move.  When you have just one "
		     // "slice blinking, that is the one that will move."),
		// i18n("Move Error"), "move_error_2");
	// }
	// else if (result > 1) {
	    // The mouse always stayed on the same cubie.
	    // KMessageBox::information (myParent,
		// i18n("To turn a slice of the cube you should drag "
		     // "the mouse pointer from one colored sticker to "
		     // "another, with the left button held down.  If you "
		     // "stay on the same sticker, nothing happens."),
		// i18n("Move Error"), "move_error_3");
	// }
    } // End - Button-release event
}


bool MoveTracker::findFaceCentre (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int mX, int mY)
{
    int    face [nAxes];
    bool   found = false;

    // Read the depth value at the position on the screen.
    GLfloat depth;
    glReadPixels (mX, mY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    // If playing, find which picture of a cube the mouse is on.
    // IDW *** CubeView * v = cubeViews.at
    v = cubeViews.at (findWhichCube (sceneID, cubeViews, mX, mY, depth));
    if (v == 0) {
	// Could not find the nearest cube (should never happen).
	return false;
    }

    // Get the mouse position relative to the centre of the cube we found.
    getGLPosition (mX, mY, depth, v->matrix, position);

    // Find the sticker we are on (ie. the closest face-centre).
    found = cube->findSticker (position, v->cubieSize, face);
    // IDW kDebug() << "Found position:" << position[0] << position[1] << position[2]
		// IDW << "face:" << face[0] << face[1] << face[2];

    if (found) {
	if (event == ButtonDown) {
	    clickFace1 = true;
	    foundFace1 = true;
	    LOOP (n, nAxes) {
		face1 [n] = face [n];
	    }
	}
	else {
	    // Ignore the second face centre if it is the same as the first.
	    int count = 0;
	    if (clickFace1) {
		LOOP (n, nAxes) {
		    if (face1 [n] == face [n]) {
			count++;
		    }
		}
	    }
	    if (count == nAxes) {
		foundFace2 = false;
		found = false;
	    }
	    else {
		foundFace2 = true;
		LOOP (n, nAxes) {
		    face2 [n] = face [n];
		}
		if (! clickFace1) {
		    // We have run onto the cube: calculate a pseudo-face.
		    found = findPseudoFace (face, mX1, mY1, v, cube, face1);
		    foundFace1 = true;
		    kDebug() << "Pseudo face 1:" <<
			position[0] << position[1] << position[2] <<
			"face:" << face1[0] << face1[1] << face1[2];
		}
	    }
	}
    }
    else {
	if (clickFace1) {
	    // We have run off the edge of the cube.  If we have no second face
	    // yet, calculate a pseudo-face, otherwise keep the faces we have.
	    if (! foundFace2) {
		found = findPseudoFace (face1, mX, mY, v, cube, face2);
		foundFace2 = true;
		kDebug() << "Pseudo face 2:" <<
			position[0] << position[1] << position[2] <<
			"face:" << face2[0] << face2[1] << face2[2];
	    }
	}
	else {
	    // We did not start on the cube: just remember the last (mX, mY).
	    foundFace1 = false;
	    foundFace2 = false;
	    mX1 = mX;
	    mY1 = mY;
	}
    }

    // IDW kDebug() << "Found:" << found << "clickFace1:" << clickFace1 <<
		// IDW "foundFace1:" << foundFace1 << "foundFace2:" << foundFace2;
    return found;
}


bool MoveTracker::findPseudoFace (int realFace [], int mouseX, int mouseY,
				CubeView * v, Cube * cube, int pseudoFace [])
{
    // Get two points on line of sight, in the nearest cube's co-ordinates.
    double point1 [nAxes];
    double point2 [nAxes];

    // The "depth" in OpenGL is a normalised number in the range 0.0 to 1.0,
    // so we choose values 0.5 and 1.0 (background) for our two depths.

    getGLPosition (mouseX, mouseY, 0.5, v->matrix, point1);
    printf ("Point1 cube coords: %7.2f %7.2f %7.2f\n",
					point1[X], point1[Y], point1[Z]);
    getGLPosition (mouseX, mouseY, 1.0, v->matrix, point2);
    printf ("Point2 cube coords: %7.2f %7.2f %7.2f\n",
					point2[X], point2[Y], point2[Z]);

    // Find where the line of sight intersects the plane of the found face.
    // To do this, we use the parametrised equation of a line between two
    // points, i.e.  p = p2 + lambda * (p1 - p2) for any point p.

    int    normal = cube->faceNormal (realFace);
    double plane  = cube->convToOpenGL (realFace [normal], v->cubieSize);
    double lambda = (plane - point2 [normal]) /
				(point1 [normal] - point2 [normal]);
    double point3 [nAxes];
    LOOP (n, nAxes) {
	if (n == normal) {
	    point3 [n] = plane;
	}
	else {
	    point3 [n] = point2 [n] + lambda * (point1 [n] - point2 [n]);
	}
	printf ("    Point3 [%d] (cube coord): %7.2f\n",
				n, (point3 [n] * 2.0) / v->cubieSize);
    }
    printf ("Lambda: %7.2f Point3: %7.2f %7.2f %7.2f\n",
				lambda, point3[X], point3[Y], point3[Z]);

    // Now we have a point in OpenGL co-ordinates, relative to the centre
    // of the cube, that is in the same plane as realFace and just outside
    // that face's sticker.  It is where the mouse was pointing.  Now we
    // wish to calculate the centre of an imaginary sticker that would lie
    // next to realFace, in the direction of the point we have found.

    cube->findPseudoFace (realFace, normal, v->cubieSize, point3, pseudoFace);
    kDebug() << "Real   face:" << realFace[0] << realFace[1] << realFace[2];
    kDebug() << "Pseudo face:" << pseudoFace[0] << pseudoFace[1] << pseudoFace[2];
    return true;
}


int MoveTracker::evaluateMove (Cube * cube)
{
    int count = 0;		// Result: 1 = OK, other values are error codes.

    if (foundFace1 && foundFace2) {
	Axis axis = X;

	// Find the axis and slice for the mouse-guided move.
	int normal2 = cube->faceNormal (face2);
	int normal1 = cube->faceNormal (face1);
	LOOP (n, nAxes) {
	    // Check axes that are perpendicular to either cubie face.
	    if (n == normal2) {
		if (n == normal1) {
		    // The mouse is on the same or the opposite face of the
		    // cube: we need the row/column of BOTH mouse posns
		    // to identify which of two slices is to move, so
		    // the mouse needs to be exactly on that row/column.
		    LOOP (n, nAxes) {
			// Find equal face-centre coords on the two cubies.
			if ((face2[n] == face1[n]) && (n != normal2)) {
			    count++;
			    axis = (Axis) n;
			}
		    }
		    break;
		}
		else {
		    // The mouse has gone around to a face of the cube
		    // that is at right angles to the first one: the 
		    // required slice is perpendicular to both faces.
		    // The mouse need not be on a cubie in that slice,
		    // so the user does not need to be so precise.

		    // Find the axis of the required slice.
		    axis = (Axis) ((n + 1) % nAxes);
		    if (axis == normal1) {
			// The slice axis must be the next axis but one.
			axis = (Axis) ((n + 2) % nAxes);
		    }
		    count = 1;
		}
	    }
	}

	if (count == 1) {
	    // The move was between two separate cubies in a slice.
	    currentMoveAxis = axis;
	    if (currentButton == Qt::LeftButton) {
		currentMoveSlice = face1[axis];
	    }
	    else {
		currentMoveSlice = WHOLE_CUBE;
	    }

	    // If the cross-product of face-centre vectors is positive
	    // along "axis", we rotate anticlockwise, else clockwise.

	    // First we compute the other two axes, in cyclical order.
	    Axis a1 = (Axis) ((currentMoveAxis + 1) % nAxes);
	    Axis a2 = (Axis) ((currentMoveAxis + 2) % nAxes);

	    if (((face1[a1] * face2[a2]) - (face2[a1] * face1[a2])) > 0) {
		currentMoveDirection = ANTICLOCKWISE;
	    }
	    else {
		currentMoveDirection = CLOCKWISE;
	    }
	}
	else {
	    // If count == 0, the move was skewed between two slices.
	    // If count > 1, the mouse always stayed on the same cubie.
	}
    }
    else {
	// We found either one cubie or none.  Either the press and release
	// both missed the cube or both were on the same sticker.
	count = (foundFace1 || foundFace2) ? 2 : -1;
    }
    return (count);
}


int MoveTracker::findWhichCube (int sceneID, QList<CubeView *> cubeViews,
					int mX, int mY, GLfloat depth)
{
    // For some reason this function cannot compile with return-type CubeView *.
    double position [nAxes];
    double distance = 10000.0;		// Large value.
    double d        = 0.0;
    double dx       = 0.0;
    double dy       = 0.0;
    double dz       = 0.0;
    int    indexV   = -1;

    // Get the mouse position in OpenGL world co-ordinates.
    getAbsGLPosition (mX, mY, depth, position);

    // Find which cube in the current scene is closest to the mouse position.
    CubeView * v;
    LOOP (n, cubeViews.size()) {
        v = cubeViews.at (n);
	if (v->sceneID != sceneID)
	    continue;			// Skip unwanted scene IDs.

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
	// IDW *** std::cerr << "gluUnProject() did not succeed" << std::endl;
	return;
    }

    // Return the OpenGL coordinates we found.
    pos[X] = objx; pos[Y] = objy; pos[Z] = objz;
}


void MoveTracker::usersRotation()
{
    glMultMatrixf (rotationMatrix);
}
