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

#ifndef CUBE_H
#define CUBE_H

#include <QObject>

#include "kbkglobal.h"

class GameGLView;

class Cubie;		// Forward declaration of Cube's component.

// Face-sticker colors are in the order of axes X/Y/Z then -ve/+ve direction.
// INTERNAL is the color of the material of which the cube is made (eg. gray).
enum	FaceColor	{INTERNAL, LEFT, RIGHT, BOTTOM, TOP, BACK, FRONT};

/** 
 * The Cube class represents a Rubik's Cube in an abstract way.
 *
 * Actually the "cube" is a rectangular parallelepiped, which can have unequal
 * sides, like a brick, or can be one layer thick, like a mat.  The cube is
 * made up of many "cubies" (small cubes) stacked in a 3-D array.  The original
 * Rubik's Cube was an array of 3x3x3 cubies, but in this version the sides can
 * have any number of cubies between 1 and 6, but only one side can be 1 cubie
 * long, otherwise the puzzle becomes too easy (e.g. the dimensions can be
 * 1x2x2, 6x6x6, 4x1x3, 3x4x5, etc.).
 *
 * The six faces of the cube are covered in stickers of six different colors.  
 * Cubies are dark grey, but can have stickers on one, two or three of their
 * six faces, depending on whether they are located in the middle of a cube
 * face, on a cube edge or at a cube corner.  If all cube dimensions are 3 or
 * more, some cubies will be hidden and will have no stickers (e.g. a 3x3x3
 * cube has one hidden cubie).  To save time when handling large cubes, we do
 * not draw hidden cubies.
 *
 * The cube can be "shuffled" by rotating "slices" of cubies that lie in the
 * same plane, analogous to slices of a loaf of bread.  The faces of the cube
 * then become a jumble of colors.
 *
 * The object of the game is to "solve" a shuffled cube, by selecting and
 * rotating slices in sequence so that all the cube faces end up with their
 * original colors.
 *
 * This class holds the current position of the cube, using an abstract set
 * of co-ordinates that simplify procedures such as creating, painting and
 * moving a cube.  Firstly, all cubies are of size 2x2x2.  This means that
 * the centre of a cubie always has integer co-ordinates, regardless of
 * whether the number of cubies in a side is even or odd.
 *
 * The origin of the co-ordinates is at the centre of gravity of the cube and
 * the centres of the cubies have positive and negative co-ordinates running
 * from there.  Looking at one co-ordinate only of a line of cubies, we have:
 *
 *     Centres of 3 cubies: ... |-2 | 0 |+2 | ... End faces at -3 and +3
 *
 *     Centres of 4 cubies: . |-3 |-1 |+1 |+3 | . End faces at -4 and +4
 *
 * Note that the origin (zero) is between two cubies when the number of cubies
 * in the line is even.  More importantly, note that the end faces of the line,
 * which will be have stickers, are always at -N and +N, where N is the number
 * of cubies in the line.  In a full cube, consisting of LxMxN cubies, the six
 * colored faces are at distances -L, +L, -M, +M, -N and +N from the origin.
 *
 * A slice (see above) is represented as all cubies whose centres have the same
 * value in one of the co-ordinates.  That value also gives the location of the
 * slice and the axis around which to rotate the slice.  For example, in a
 * 3x3x3 cube, the slice containing the right-hand face will consist of all
 * cubies whose centres have X-coordinate = +2 and the axis around which the
 * slice rotates is the X-axis itself, which runs from left to right through
 * the centre of the cube (the origin of co-ordinates).
 *
 * Finally, looking at the screen, the X-axis runs from left to right, the
 * Y-axis runs from bottom to top (as in mathematics) and the Z-axis runs from
 * back to front (out of the screen towards you), which is as in OpenGL library
 * usage.  Last but not least, the co-ordinates of points such as the centre of
 * a cubie are stored in arrays of size 3 (e.g. the X, Y and Z co-ordinates of
 * a point P are in array elements P[0], P[1] and P[3] and the X, Y and Z axes
 * are now the "0", "1" and "2" axes.
 *
 * This is handy because just two numbers, an axis number and a co-ordinate
 * value can represent any slice or face of the cube.  For example the top and
 * bottom faces of a 3x3x3 cube are (1,3) and (1,-3) and the central horizontal
 * slice is (1,0), representing the planes Y = +3 and Y = -3 and the 9 cubies
 * whose centres have Y = 0.
 */
class Cube : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor for the Cube object
     * @param parent	The parent widget
     * @param xlen	The number of cubies in the X direction (left to right)
     * @param ylen	The number of cubies in the Y direction (bottom to top)
     * @param zlen	The number of cubies in the Z direction (back to front)
     */
    Cube (QObject * parent = 0, int xlen = 3, int ylen = 3, int zlen = 3);
    ~Cube();

    void drawCube    (GameGLView * gameGLView, float cubieSize, int moveAngle);
    void moveSlice   (Axis axis, int location, Rotation direction);

    void setNextMove       (Axis axis, int location);
    void setBlinkingOn     (Axis axis, int location);
    void setBlinkingOff    ();

    bool findSticker (double position [nAxes], float myCubieSize,
				int faceCentre [nAxes]);
    int  faceNormal        (int faceCentre [3]);
    double convToOpenGL    (int internalCoord, double cubieSize);
    void findPseudoFace    (int realFace[3], int normal, double cubieSize,
				double point3[3], int pseudoFace[3]);

private:
    void addStickers ();		// Add colored stickers to the faces.

    int   sizes [nAxes];		// The number of cubies on each axis.
    QList<Cubie *>	cubies;		// The list of cubies in the cube.

    Axis  nextMoveAxis;
    int   nextMoveSlice;
};

class Cubie : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor for the Cubie object
     * @param centre	The co-ordinates of the central point (int [nAxes])
     */
    Cubie (int centre [nAxes]);
    ~Cubie ();

    void rotate (Axis axis, int location, Rotation direction);

    void addSticker (FaceColor color, Axis axis, int location, int sign);

    bool hasNoStickers ();

    void drawCubie (GameGLView * gameGLView, float cubieSize,
		    int nextMoveAxis, int nextMoveSlice, int angle);

    double findCloserSticker (double distance, double location [],
			      int faceCentre []);
    void setBlinkingOn  (Axis axis, int location, int cubeBoundary);
    void setBlinkingOff ();

    void printAll ();
    void printChanges ();

private:
    int	originalCentre [nAxes];		// Original location of the cubie.
    int	currentCentre  [nAxes];		// Current location of the cubie.

    typedef struct {			// Define type "Sticker".
	FaceColor color;
	bool      blinking;
	int originalFaceCentre [nAxes];
	int currentFaceCentre  [nAxes];
    } Sticker;

    QList<Sticker *>	stickers;	// The stickers on the cubie (if any).
};

#endif	// CUBE_H
