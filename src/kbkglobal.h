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

enum	Axis		{X, Y, Z, nAxes};
enum	Rotation	{ANTICLOCKWISE, CLOCKWISE, ONE_EIGHTY};
#define	WHOLE_CUBE	99
enum	Option		{optXDim, optYDim, optZDim, optShuffleMoves,
			 optViewShuffle, optViewMoves, optMoveSpeed,
			 optBevel, optSceneID, optTumbling, optTumblingTicks,
			 optMouseBlink, nOptions};
enum	SceneID		{OneCube = 1, TwoCubes, ThreeCubes, nSceneIDs};
#define	TURNS		TRUE
#define	FIXED		FALSE
enum	LabelID		{NoLabel, DemoLbl, FrontLbl, BackLbl};

#endif	// KBK_GLOBAL_H
