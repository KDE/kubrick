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

#ifndef KBK_MOVETRACKER_H
#define KBK_MOVETRACKER_H

#include <QObject>
#include <QList>
#include <QPoint>

#include "kbkglobal.h"
#include "cube.h"

class QWidget;

class MoveTracker : public QObject
{
  Q_OBJECT

public:
    /**
     * Constructor for the Move Tracker object.
     * @param parent	The parent widget.
     */
    MoveTracker (QWidget * parent = 0);

    ~MoveTracker();

    void    init();

    /**
     * Procedure called by Game to handle mouse-input.  It calculates the
     * cube-picture and sticker that the mouse is pointing to.  Then it
     * works out whether a cube-move has been completed.
     */
    void    mouseInput (int sceneID, QList<CubeView *> cubeViews,
		Cube * cube, MouseEvent event, int button, int mX, int mY);
signals:
    /**
     * This signal is used to pass a move back to the Game object, after the
     * MoveTracker has found such a move by tracking the mouse.
     */
    void    newMove (Move * move);

private:
    bool    findFaceCentre (int sceneID, QList<CubeView *> cubeViews,
				Cube * cube, MouseEvent event, int mX, int mY);
				// Cube * cube, MouseEvent event, int mX, int mY, int face[]);
    int     findWhichCube (int sceneID, QList<CubeView *> cubeViews,
				int mX, int mY, GLfloat depth);
    bool    findPseudoFace (int realFace [], int mouseX, int mouseY,
				CubeView * v, Cube * cube, int pseudoFace []);
    int     evaluateMove (Cube * cube);
    void    getAbsGLPosition (int sX, int sY, GLfloat depth, double pos[nAxes]);
    void    getGLPosition (int sX, int sY, GLfloat depth,
					double matrix[16], double pos[nAxes]);

    QWidget * myParent;

    int     mX1, mY1;		// Last mouse co-ordinates when off-cube.
    bool    clickFace1;		// True if left mouse-button press found a face.
    int     face1 [nAxes];	// The centre of the starting face of the move.
    int     face2 [nAxes];	// The centre of the finishing face of the move.
    bool    foundFace1;		// True if starting face has been found.
    bool    foundFace2;		// True if finishing face has been found.
    int     currentButton;	// The button that is being pressed (if any).

    bool    blinking;		// Feedback of a move is being shown.

    // The move the player is selecting or has just selected.
    Axis    currentMoveAxis;
    int     currentMoveSlice;
    Rotation currentMoveDirection;
};

#endif	// KBK_MOVETRACKER_H
