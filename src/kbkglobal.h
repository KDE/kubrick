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

#ifndef KBK_GLOBAL_H
#define KBK_GLOBAL_H

// MACRO - Loop for i = 0 to (n-l).
#define LOOP(i,n) for(int ((i))=0; ((i))<((n)); ((i))++)

#ifdef WIN32
#include <windows.h>  // Needed to avoid errors when including OpenGL headers.
#endif

#include <GL/gl.h>	// Make OpenGL library and types available globally.
#include <GL/glu.h>

enum	Axis		{X, Y, Z, nAxes};
enum	Rotation	{ANTICLOCKWISE, CLOCKWISE, ONE_EIGHTY};
#define	WHOLE_CUBE	99

enum	SingmasterMove	{SM_UP, SM_DOWN, SM_LEFT, SM_RIGHT, SM_FRONT, SM_BACK,
			 SM_ANTICLOCKWISE, SM_DOUBLE, SM_2_SLICE, SM_ANTISLICE,
			 SM_INNER, SM_EXECUTE, SM_CUBE, SM_SPACER};
const char SingmasterNotation [] = {"UDLRFB'2+-.xC "};

enum	Option		{optXDim, optYDim, optZDim, optShuffleMoves,
			 optViewShuffle, optViewMoves, optMoveSpeed,
			 optBevel, optSceneID, optTumbling, optTumblingTicks,
			 optMouseBlink, nOptions};
enum	SceneID		{OneCube = 1, TwoCubes, ThreeCubes, nSceneIDs};
#define	TURNS		true
#define	FIXED		false
enum	LabelID		{NoLabel, DemoLbl, FrontLbl, BackLbl};

enum	MouseEvent	{ButtonDown, Tracking, ButtonUp};

#define viewAngle       30.0	// Angle of field of view in Y direction.
#define minZ             1.0	// Nearest point represented in the view.
#define maxZ            20.0	// Furthest point represented in the view.

#define cubeCentreZ	-5.0	// Z-value for all centres of cubes.

#define defaultOwnMove	23	// Default degrees/frame for own moves.

typedef struct {		// Define type "CubeView".
	int		sceneID;	// Scene ID (1, 2, or 3 cubes in scene).
	bool		rotates;	// True if user can rotate this cube.
	float		size;		// Overall size in GL co-ordinates.
	float           relX;		// Relative X-position of centre.
	float           relY;		// Relative Y-position of centre.
	float		position [nAxes]; // GL co-ordinates of centre of cube.
	float		turn;		// Turn angle around Y axis.
	float		tilt;		// Tilt angle.
	GLdouble	matrix0 [16];	// GL matrix for home position of cube.
	GLdouble	matrix  [16];	// GL matrix for current position.
	float		cubieSize;	// Size of cubies (for findSticker()).
	int		labelX;		// Label X posn. in 1/8ths widget size.
	int		labelY;		// Label Y posn. in 1/8ths widget size.
	LabelID		label;		// Index of label object and text.
} CubeView;

typedef struct {		// Define type "Move".
	Axis		axis;		// Axis of rotation.
	int		slice;		// Slice to be rotated.
	Rotation	direction;	// Direction to move.
	int		degrees;	// Angle of move (90 or 180 degrees).
} Move;

#endif	// KBK_GLOBAL_H
