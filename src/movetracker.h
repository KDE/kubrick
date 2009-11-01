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

#ifndef KBK_MOVETRACKER_H
#define KBK_MOVETRACKER_H

#include <QObject>
#include <QList>
#include <QPoint>

#include "kbkglobal.h"
#include "cube.h"
#include "quaternion.h"

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

    void    usersRotation();	// Perform the user's rotation when rendering.

    void    realignCube		// Realign the cube to nearest orthogonal axes.
		(QList<Move *> & tempMoves);

signals:
    /**
     * This signal is used to pass a move back to the Game object, after the
     * MoveTracker has found such a move by tracking the mouse.
     */
    void    newMove (Move * move);

    /**
     * This signal is used to tell the Game object that the user has rotated the
     * cube manually and it must be realigned before the next keyboard move.
     */
    void    cubeRotated();

private:
    void    trackCubeRotation (int sceneID, QList<CubeView *> cubeViews,
			MouseEvent event, int mX, int mY);
    bool    calculateRotation (const int mX, const int mY,
			double axis [nAxes], double & degrees);
    void    trackSliceMove (int sceneID, QList<CubeView *> cubeViews,
			Cube * cube, MouseEvent event, int mX, int mY);

    int     findWhichCube (const int sceneID, const QList<CubeView *> cubeViews,
			const double position[]);

    void    makeWholeCubeMoveList (QList<Move *> & tempMoves,
			const double to [nAxes * nAxes]);
    void    prepareWholeCubeMove (QList<Move *> & moveList,
			int to [nAxes][nAxes], const Axis a, const Rotation d);
    void    rotateAxes (const Quaternion & r, const double from[nAxes * nAxes],
			double to[nAxes * nAxes]);
    bool    getTurnVector (const double u1[nAxes], const double u2[nAxes],
			double turnAxis[nAxes], double & turnAngle);

    GLfloat getMousePosition (const int mX, const int mY, double pos[]);
    void    getAbsGLPosition (int sX, int sY, GLfloat depth, double pos[nAxes]);
    void    getGLPosition (int sX, int sY, GLfloat depth,
			double matrix[16], double pos[nAxes]);

    QWidget * myParent;

    CubeView * v;		// The cube-view that is being rotated/moved.

    double  position [nAxes];	// Mouse co-ords relative to centre of cube.
    double  handle   [nAxes];	// Mouse co-ords at the start of a rotation.
    bool    foundHandle;	// True if a rotation-handle has been found.
    double  R;			// Radius of handle-sphere.
    double  RR;			// Square of radius of handle-sphere.

    int     mX0, mY0;		// Mouse co-ordinates at the handle position.
    int     mX1, mY1;		// Last mouse co-ordinates when off-cube.
    int     noTurn;		// Selected slice cannot turn around this axis.
    bool    clickFace1;		// True if left mouse-button press found a face.
    int     face1 [nAxes];	// The centre of the starting face of the move.
    int     face2 [nAxes];	// The centre of the finishing face of the move.
    bool    foundFace1;		// True if starting face has been found.
    bool    foundFace2;		// True if finishing face has been found.
    int     currentButton;	// The button that is being pressed (if any).
    int     moveAngle;		// The angle that shows the next slice move.

    // The move the player is selecting or has just selected.
    Axis    currentMoveAxis;
    int     currentMoveSlice;
    Rotation currentMoveDirection;

    // Data that keeps track of the user's rotations of the whole cube.
    Quaternion rotationState;	// The combination of all the user's rotations.
    Matrix     rotationMatrix;	// The corresponding OpenGL rotation matrix.
};

#endif	// KBK_MOVETRACKER_H
