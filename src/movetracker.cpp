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
}


MoveTracker::~MoveTracker()
{
}


void MoveTracker::init()
{
    currentButton = Qt::NoButton;	// No mouse button being pressed.
    clickFace1 = false;			// No face clicked by mouse button.
    foundFace1 = false;			// Starting face of move not found.
    foundFace2 = false;			// Finishing face of move not found.
}


void MoveTracker::mouseInput (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int button, int mX, int mY)
{
    bool found = findFaceCentre (sceneID, cubeViews, cube, event, mX, mY);
    // After a button-press, save the cubie face-centre that was found (if any).
    if (event == ButtonDown) {
	currentButton = button;
	if (found) {
	    cube->setBlinkingOff ();
	    blinking = false;
	    int normal = cube->faceNormal (face1);
	    LOOP (n, nAxes) {
		if (n != normal) {
		    if (currentButton == Qt::LeftButton) {
			// Show the two slices that might move.
			cube->setBlinkingOn ((Axis) n, face1[n]);
		    }
		    else {
			cube->setBlinkingOn ((Axis) n, WHOLE_CUBE);
		    }
		}
	    }
	}
    } // End - Button-press event

    if (event == Tracking) {
	cube->setBlinkingOff ();
	if (evaluateMove (cube) == 1) {
	    // We have identified a single slice: make it blink.
            // IDW kDebug() << "Blink axis:" << currentMoveAxis
			// IDW << "slice:" << currentMoveSlice;
	    cube->setBlinkingOn (currentMoveAxis, currentMoveSlice);
	}
	else if (clickFace1) {
	    // Cannot identify a single slice: make two possible slices blink.
	    int normal1 = cube->faceNormal (face1);
	    LOOP (n, nAxes) {
		if (n != normal1) {
		    if (currentButton == Qt::LeftButton) {
		    // IDW kDebug() << "Blink 2 axes:" << n << "slice:" << face1[n];
		    // Show the two slices that might move.
		    cube->setBlinkingOn ((Axis) n, face1[n]);
		    }
		    else {
		    // IDW kDebug() << "Blink axis:" << n << "WHOLE_CUBE";
		    // Show the two slices that might move.
		    cube->setBlinkingOn ((Axis) n, WHOLE_CUBE);
		    }
		}
	    }
	}
    } // End - Tracking event

    // After a button-release, calculate the slice or whole-cube move required.
    if (event == ButtonUp) {
	int result = evaluateMove (cube);
	cube->setBlinkingOff ();
	blinking = false;
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
	else if (result < 0) {
	    // We found no cubies.
	    // KMessageBox::information (myParent,
		// i18n("You should start or finish with the mouse "
		     // "pointer on a colored sticker, not in the "
		     // "background area."),
		// i18n("Move Error"), "move_error_1");
	}
	else if (result == 0) {
	    // The move was skewed between two slices.
	    KMessageBox::information (myParent,
		i18n("To turn a slice of the cube you should hold "
		     "the left mouse button down and drag the pointer "
		     "from one colored sticker to another on the "
		     "same slice, or you can go around onto "
		     "another face of the cube.\n\nIf you try your move "
		     "again, but slowly, the cube will blink and show "
		     "you which slices can move.  When you have just one "
		     "slice blinking, that is the one that will move."),
		i18n("Move Error"), "move_error_2");
	}
	else if (result > 1) {
	    // The mouse always stayed on the same cubie.
	    // KMessageBox::information (myParent,
		// i18n("To turn a slice of the cube you should drag "
		     // "the mouse pointer from one colored sticker to "
		     // "another, with the left button held down.  If you "
		     // "stay on the same sticker, nothing happens."),
		// i18n("Move Error"), "move_error_3");
	}
	currentButton = Qt::NoButton;

    } // End - Button-release event
}


bool MoveTracker::findFaceCentre (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int mX, int mY)
{
    double position [nAxes];
    int    face [nAxes];
    bool   found = false;
    if (event == ButtonDown) {
	init();
    }

    // Read the depth value at the position on the screen.
    GLfloat depth;
    glReadPixels (mX, mY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    // If playing, find which picture of a cube the mouse is on.
    CubeView * v = cubeViews.at
			(findWhichCube (sceneID, cubeViews, mX, mY, depth));
    if (v == 0) {
	// Could not find the nearest cube (should never happen).
	return false;
    }

    // Get the mouse position relative to the centre of the cube we found.
    getGLPosition (mX, mY, depth, v->matrix, position);

    // Find the sticker we are on (ie. the closest face-centre).
    found = cube->findSticker (position, v->cubieSize, face);
    kDebug() << "Found position:" << position[0] << position[1] << position[2]
		<< "face:" << face[0] << face[1] << face[2];

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

    kDebug() << "Found:" << found << "clickFace1:" << clickFace1 <<
		"foundFace1:" << foundFace1 << "foundFace2:" << foundFace2;
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

