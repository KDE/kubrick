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

// The RubikCube object uses the sqrt() function.
#include <math.h>
#include <stdlib.h>

// Local includes
#include "gameglview.h"
#include "cube.h"

// Create a Cube

Cube::Cube (QObject * parent, int xlen, int ylen, int zlen)
	: QObject (parent)
{
    sizes [X] = xlen;
    sizes [Y] = ylen;
    sizes [Z] = zlen;

    // Generate a list of all the cubies in the cube.
    // Set the centres of cubies at +ve and -ve co-ords around (0,0,0),
    // as documented in "game.h".  The cubie dimensions are 2x2x2.  Each
    // cubie's centre co-ordinate is (2*index - end-face-pos + 1).
    int centre [nAxes];

    while (! cubies.isEmpty()) {
        delete cubies.takeFirst();
    }
    LOOP (i, sizes [X]) {
	centre [X] = 2*i - sizes [X] + 1;
	LOOP (j, sizes [Y]) {
	    centre [Y] = 2*j - sizes [Y] + 1;
	    LOOP (k, sizes [Z]) {
		centre [Z] = 2*k - sizes [Z] + 1;
		cubies.append (new Cubie (centre));
	    }
	}
    }

    addStickers ();		// Add colored stickers to the faces.

    setBlinkingOff ();
    moveInProgressAxis   = Z;	// Front face (+Z).
    moveInProgressSlice  = sizes[Z] - 1;
    moveInProgressAngle  = 0;
}

Cube::~Cube ()
{
    while (! cubies.isEmpty()) {
        delete cubies.takeFirst();
    }
}

void Cube::moveSlice (Axis axis, int location, Rotation direction)
{
    // If single-slice and not square, rotate 180 degrees rather than 90.
    if ((location != WHOLE_CUBE) &&
	(sizes [(axis + 1)%nAxes] != sizes [(axis + 2)%nAxes])) {
	direction = ONE_EIGHTY;
    }

    // Rotate all cubies that are in the required slice.
    foreach (Cubie * cubie, cubies) {
	cubie->rotate (axis, location, direction);
    }
    setBlinkingOff ();
}

void Cube::addStickers ()
{
    int color = INTERNAL;			// ie. Zero.

    // Add stickers to cube faces in the order of axes X/Y/Z then -ve/+ve end.
    LOOP (n, nAxes) {
	LOOP (minusPlus, 2) {
	    int sign = 2*minusPlus - 1;		// sign = -1 or +1.
	    int location = sign * sizes [n];

	    color++;				// FaceColor enum 1 --> 6.
	    foreach (Cubie * cubie, cubies) {
		cubie->addSticker ((FaceColor) color, (Axis) n, location, sign);
	    }
	}
    }
}


void Cube::drawCube (GameGLView * gameGLView, float cubieSize)
{
    // For each cubie in the cube ...
    foreach (Cubie * cubie, cubies) {

	if (cubie->hasNoStickers()) {
	    // This cubie is deep inside the cube: save time by not drawing it.
	    continue;
	}

	// Draw the cubie and its stickers.
	cubie->drawCubie (gameGLView, cubieSize,
		  moveInProgressAxis, moveInProgressSlice, moveInProgressAngle);
    }
}


bool Cube::findSticker (double position [], float myCubieSize,
				int faceCentre [])
{
    bool             result       = false;
    double           location [nAxes];
    double           distance     = sqrt ((double) 2.0);
    double	     d;

    // Calculate the position in the cube's internal co-ordinate system.
    LOOP (i, nAxes) {
	location [i] = (position [i] / myCubieSize) * 2.0;
	// IDW faceCentre [i] = 0;		// Return zeroes if no sticker is found.
    }

    foreach (Cubie * cubie, cubies) {
	d = cubie->findCloserSticker (distance, location, faceCentre);
	if (d < distance) {
	    distance = d;
	    result = true;
	}
    }

    return (result);
}


void Cube::setMoveInProgress (Axis axis, int location)
{
    setBlinkingOff ();
    moveInProgressAxis   = axis;
    moveInProgressSlice  = location;
}


void Cube::setMoveAngle (int angle)
{
    moveInProgressAngle  = angle;
}


void Cube::setBlinkingOn (Axis axis, int location)
{
    foreach (Cubie * cubie, cubies) {
	cubie->setBlinkingOn (axis, location, sizes[axis]);
    }
}


void Cube::setBlinkingOff ()
{
    foreach (Cubie * cubie, cubies) {
	cubie->setBlinkingOff ();
    }
}


int Cube::faceNormal (int faceCentre [3])
{
    LOOP (i, nAxes) {
	if (abs(faceCentre [i]) == sizes [i]) {
	    return i;
	}
    }
    return 0;
}


double Cube::convToOpenGL (int internalCoord, double cubieSize)
{
    return ((double) internalCoord / 2.0) * cubieSize;
}

Cubie::Cubie (int centre [nAxes])
{
    LOOP (i, nAxes) {
	originalCentre [i] = centre [i];
	currentCentre  [i] = centre [i];
    }
}


Cubie::~Cubie ()
{
    while (! stickers.isEmpty()) {
        delete stickers.takeFirst();
    }
}


void Cubie::rotate (Axis axis, int location, Rotation direction)
{
    // Cubie moves only if it is in the required slice or in a whole-cube move.
    if ((location != WHOLE_CUBE) && (currentCentre [axis] != location)) {
	return;
    }

    // The co-ordinate on the axis of rotation does not change, but we must
    // work out what the other two co-ordinates are, in cyclical order: i.e.
    //         X-axis (0) --> Y,Z (co-ordinates 1 and 2 change),
    //         Y-axis (1) --> Z,X (co-ordinates 2 and 0 change),
    //         Z-axis (2) --> X,Y (co-ordinates 0 and 1 change). 
    //
    Axis coord1 = (Axis) ((axis + 1) % nAxes);
    Axis coord2 = (Axis) ((axis + 2) % nAxes);
    int  temp;

    switch (direction) {
	case (ANTICLOCKWISE):	// eg. around the Z-axis, X --> Y and Y --> -X.
	    temp   = currentCentre [coord1];
	    currentCentre [coord1] = - currentCentre [coord2];
	    currentCentre [coord2] = + temp;
	    foreach (Sticker * s, stickers) {
		temp   = s->currentFaceCentre [coord1];
		s->currentFaceCentre [coord1] = - s->currentFaceCentre [coord2];
		s->currentFaceCentre [coord2] = + temp;
	    }
	    break;
	case (CLOCKWISE):	// eg. around the Z-axis, X --> -Y and Y --> X.
	    temp   = currentCentre [coord1];
	    currentCentre [coord1] = + currentCentre [coord2];
	    currentCentre [coord2] = - temp;
	    foreach (Sticker * s, stickers) {
		temp   = s->currentFaceCentre [coord1];
		s->currentFaceCentre [coord1] = + s->currentFaceCentre [coord2];
		s->currentFaceCentre [coord2] = - temp;
	    }
	    break;
	case (ONE_EIGHTY):	// eg. around the Z-axis, X --> -X and Y --> -Y.
	    currentCentre [coord1] = - currentCentre [coord1];
	    currentCentre [coord2] = - currentCentre [coord2];
	    foreach (Sticker * s, stickers) {
		s->currentFaceCentre [coord1] = - s->currentFaceCentre [coord1];
		s->currentFaceCentre [coord2] = - s->currentFaceCentre [coord2];
	    }
	    break;
	default:
	    break;
    }
}


void Cubie::addSticker (FaceColor color, Axis axis, int location, int sign)
{
    // The cubie will get a sticker only if it is on the required face.
    if (originalCentre [axis] != (location - sign)) {
	return;
    }

    // Create a sticker.
    Sticker * s = new Sticker;
    s->color    = color;
    s->blinking = false;
    LOOP (n, nAxes) {
	// The co-ordinates not on "axis" are the same as at the cubie's centre.
	s->originalFaceCentre [n] = originalCentre [n];
	s->currentFaceCentre  [n] = originalCentre [n];
    }

    // The co-ordinate on "axis" is offset by -1 or +1 from the cubie's centre.
    s->originalFaceCentre [axis] = location;
    s->currentFaceCentre  [axis] = location;

    // Put the sticker on the cubie.
    stickers.append (s);
}


bool Cubie::hasNoStickers ()
{
    return (stickers.isEmpty ());
}


void Cubie::drawCubie (GameGLView * gameGLView, float cubieSize,
				Axis axis, int slice, int angle)
{
    float centre     [nAxes];

    // Calculate the centre of the cubie in OpenGL co-ordinates.
    LOOP (i, nAxes) {
	centre [i] = ((float) currentCentre [i]) * cubieSize / 2.0;
    }

    // If this cubie is in a moving slice, set its animation angle.
    int   myAngle = 0;
    if ((angle != 0) && ((slice == WHOLE_CUBE) ||
			 (currentCentre [axis] == slice))) {
	myAngle = angle;
    }

    // Draw this cubie in color zero (grey plastic color).
    gameGLView->drawACubie (cubieSize, centre, axis, myAngle);

    float faceCentre [nAxes];
    int   faceNormal [nAxes];

    // For each sticker on this cubie (there may be 0->3 stickers) ...
    foreach (Sticker * sticker, stickers) {

	// Calculate the integer unit-vector normal to this sticker's face
	// and the centre of the face, in floating OpenGL co-ordinates.
	LOOP (j, nAxes) {
	    faceNormal [j] = sticker->currentFaceCentre [j] - currentCentre [j];
	    faceCentre [j] = ((float) sticker->currentFaceCentre [j]) *
					    cubieSize / 2.0;
	}

	// Draw this sticker in the required color, blink-intensity and size.
	gameGLView->drawASticker (cubieSize, (int) sticker->color,
			  sticker->blinking, faceNormal, faceCentre);
    }

    // If cubie is moving, re-align the OpenGL axes with the rest of the cube.
    if (myAngle != 0) {
	gameGLView->finishCubie ();
    }
}


double Cubie::findCloserSticker (double distance, double location [],
				 int faceCentre [])
{
    double    len          = 0.0;
    double    dmin         = distance;
    Sticker * foundSticker = 0;

    foreach (Sticker * sticker, stickers) {
	double d = 0.0;
	LOOP (n, nAxes) {
	    len = location[n] - sticker->currentFaceCentre[n];
	    d   = d + len * len;
	}
	d = sqrt (d);
	if (d < dmin) {
	    dmin = d;
	    foundSticker = sticker;
	}
    }

    if (foundSticker != 0) {
	LOOP (n, nAxes) {
	    faceCentre[n] = foundSticker->currentFaceCentre[n];
	}
    }

    return (dmin);
}


void Cubie::setBlinkingOn (Axis axis, int location, int cubeBoundary)
{
    // Exit if the cubie is not in the slice that is going to move.
    if ((location != WHOLE_CUBE) && (currentCentre [axis] != location)) {
	return;
    }

    // If the sticker is on the outside edges of the slice, make it blink, but
    // not if it is perpendicular to the move-axis (ie. on the slice's face).
    foreach (Sticker * sticker, stickers) {
	if (abs(sticker->currentFaceCentre [axis]) != cubeBoundary) {
	    sticker->blinking = true;
	}
    }
}


void Cubie::setBlinkingOff ()
{
    foreach (Sticker * sticker, stickers) {
	sticker->blinking = false;
    }
}


void Cubie::printAll ()
{
    printf ("%2d %2d %2d -> %2d %2d %2d Stickers: ",
	    originalCentre[X], originalCentre[Y], originalCentre[Z],
	    currentCentre[X],  currentCentre[Y],  currentCentre[Z]);

    if (stickers.isEmpty ()) {
	printf ("<NONE>\n");
    }
    else {
	foreach (Sticker * sticker, stickers) {
	    printf ("<%d> at ", (int) sticker->color);
	    LOOP (n, nAxes) {
		printf ("%2d ", sticker->currentFaceCentre [n]);
	    }
	}
	printf ("\n");
    }
}


void Cubie::printChanges ()
{
    bool moved = false;

    // Check if the cubie's centre is in a new position.
    LOOP (i, nAxes) {
	if (currentCentre  [i] != originalCentre [i])
	    moved = true;
    }

    // Check if the cubie is back where it was but has been given a twist.
    if (! moved) {
	foreach (Sticker * s, stickers) {
	    LOOP (i, nAxes) {
		if (s->currentFaceCentre [i] != s->originalFaceCentre [i])
		    moved = true;
	    }
	}
    }

    // If anything has changed, print the cubie.
    if (moved) {
	printAll ();
    }
}
