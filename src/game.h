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

#ifndef GAME_H
#define GAME_H

// The RubikCube object uses the sqrt() function.
#include <math.h>
#include <stdlib.h>

// KDE includes
#include <KStandardGameAction>	// Used only to get internal names of actions.
#include <KRandomSequence>
#include <KMessageBox>
#include <KCursor>
#include <KStandardDirs>
#include <KFileDialog>

// Qt includes
#include <QObject>
#include <QWidget>
#include <QString>
#include <QList>
#include <QTimer>
#include <QLabel>

// Local includes.
#include "kubrick.h"
#include "gameglview.h"
#include "gamedialog.h"
#include "kbkglobal.h"

class Kubrick;
class GameView;		// Forward declaration of view.
class GameGLView;	// Forward declaration of OpenGL painter and view.
class RubikCube;	// Forward declaration of RubikCube.

/** 
 * This is the main Kubrick game class.
 * All information about the game itself is stored here.
 */
class Game : public QObject
{
  Q_OBJECT

public:
    /**
     * Constructor for the Kubrick game object.
     * @param parent	The parent widget, Kubrick, which inherits KMainWindow.
     */
    Game (Kubrick * parent = 0);

    ~Game();

    /**
     * Further initialisation for the Kubrick game object, which is called back
     * by the main window (Kubrick) after the GUI and OpenGL have been set up.
     * @param glv	The OpenGL widget, used to draw 3d cubes.
     * @param mw	The main window widget (identical to "parent").
     */
    void initGame (GameGLView * glv, Kubrick * mw);

    /**
     * Procedure called back by GameGLView::paintGL() to compose the current
     * game scene, which in turn calls various GameGLView methods to build up
     * pictures of cubes in whatever states are determined by the game-play.
     */
    void drawScene ();

    void saveState ();
    void restoreState ();

    void loadDemo (const QString & file);

    void setSceneLabels ();

    /**
     * Procedure called back by GameGLView to handle mouse-button events, which
     * works out the cube-picture and sticker that the mouse is pointing to.
     */
    void handleMouseEvent (int event, int button, int mX, int mY);

public slots:
    void newPuzzle              ();	// New puzzle (shuffle a similar cube).
    void undoAll                ();	// Undo all player moves (restart).

    void load			();
    void save			();
    void saveAs			();

    void changePuzzle           (const Kubrick::PuzzleItem & puzzle);
    void newCubeDialog();		// Run options dialog; build a new cube.

    void undoMove               ();	// Undo the last move.
    void redoMove               ();	// Redo an undone move.
    void toggleDemo		();	// Start/stop the demo.
    void solveCube              ();	// Show solution (reverse all moves).
    void toggleTumbling		();	// Start/stop tumbling the cube around.
    void setZeroTumbling	();	// Set cube(s) to default rotation = 0.

    void redoAll                ();	// Redo all undone moves.

    void cycleSceneUp		();	// Cycle up through the scenes.
    void cycleSceneDown		();	// Cycle down through the scenes.

    void watchShuffling		();	// Set/clear viewShuffle option.
    void watchMoves		();	// Set/clear viewMoves option.
    void enableMessages		();	// Re-enable all "don't show" messages.
    void optionsDialog		();	// Open the dialog box for game options.

    void setMoveAxis		(int i);
    void setMoveSlice		(int slice);
    void setMoveDirection	(int direction);

private slots:
    /**
    * This slot implements a game tick.  It increases the game tick counter,
    * triggers the OpenGL rendering and manages sequences of animated moves.
    * It also performs moves that are not animated, all at once, depending
    * on the current settings of the "view" options.
    **/
    void advance                ();

private:
    Kubrick *    myParent;	// Game's parent widget.
    Kubrick *    mainWindow;	// Main window: used for status, etc.
    GameGLView * gameGLView;	// OpenGL view: used to draw 3D cubes.

    typedef struct {		// Define type "CubeView".
	int		sceneID;	// Scene ID (1, 2, or 3 cubes in scene).
	bool		rotates;	// True if user can rotate this cube.
	float		size;		// Overall size in GL co-ordinates.
	float		position [nAxes]; // GL co-ordinates of centre of cube.
	float		turn;		// Turn angle around Y axis.
	float		tilt;		// Tilt angle.
	GLdouble	matrix [16];	// GL model/view matrix of this cube.
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

    KRandomSequence random;	// Random number generator object.
    RubikCube * cube;		// The Rubik's cube that is in play.
    float   cubieSize;		// Size of each cubie in OpenGL co-ordinates.
    QList<CubeView *> cubeViews; // Parameters for views of 1-3 cubes.

    QLabel * demoL;		// Text to say "DEMO - Click anywhere ...".
    QLabel * frontVL;		// Text to say "Front View".
    QLabel * backVL;		// Text to say "Back View".

/******************************************************************************/
/**************** DATA THAT RECORDS THE STATE OF PLAY *************************/
/******************************************************************************/

    QString saveFilename;	// Last filename used for Save or SaveAs.
    int     currentSceneID;	// The set of CubeViews to be displayed.
    bool    tumbling;		// If TRUE, cubes are tumbling around.

    int tumblingTicks;		// The cumulative time tumbling has been on.  In
				// effect, it records the cube's rotation state.

    int     cubeSize [nAxes];	// Size of each side of the cube (in cubies).
    int     nMax;		// Maximum side of the cube or brick.

    int     option [nOptions];	// Array of options set by GameDialog.
    bool    demoPhase;		// True while the demo is running.
    bool    viewShuffle;	// If true, animate (show) shuffling moves.
    bool    viewMoves;		// If true, animate (show) player's moves.
    int     moveSpeed;		// Degrees of turn per animation step.

    QList<Move *> moves;	// The list of moves (if any).
    int     shuffleMoves;	// The number of shuffle moves in the list.
    int     playerMoves;	// The number of player moves in the list.
    int     moveIndex;		// The index of the current move.

    // The move the player is selecting or has just selected.
    Axis    currentMoveAxis;
    int     currentMoveSlice;
    Rotation currentMoveDirection;

/******************************************************************************/
/************************* DATA TO CONTROL ANIMATION **************************/
/******************************************************************************/

    QString displaySequence;	// Codes for sequences of moves to display.
				// m = move, u = undo, r = redo, U = undo all
				// (restart puzzle), R = redo all, h = shuffle,
				// s = solve, d = demo, w = wait.

    int     nTick;		// Game-tick counter.
    bool    blinking;		// If TRUE, blink as player is selecting move.
    int     blinkStartTick;	// When to start blinking if using the mouse.

    int     movesToDo;		// Number of moves to animate in next sequence.
    bool    undoing;		// If TRUE, reverse the direction of each move.
    int     pauseTicks;		// Counter for a pause during animation.

    int     moveAngle;		// Degrees turned so far in an animated move.
    int     moveAngleStep;	// Degrees to be turned per animation step.
    int     moveAngleMax;	// Total degrees to turn in an animated move.

/******************************************************************************/
/***************************** MOUSE-CONTROL DATA *****************************/
/******************************************************************************/

    bool    foundFace1;		// True if left mouse-button press found a face.
    int     face1 [nAxes];	// The centre of the face that was found.
    int     currentButton;	// The button that is being pressed (if any).

/******************************************************************************/
/********************* METHODS TO SUPPORT GAME ACTIONS ************************/
/******************************************************************************/

    // Save the current puzzle to a KDE config file.
    void    doSave     (bool getFilename);
    void    savePuzzle (KConfig & config);

    // Load a saved puzzle from a KDE config file.
    void    loadPuzzle (KConfig & config);

    void    setCubeView (int sceneID, bool rotates, float size,
		float x, float y, float z, float turn, float tilt,
		int labelX, int labelY, LabelID label);

    void    setDefaults();	// Set default cube sizes and options.
    int     doOptionsDialog (bool changePuzzle); // Do dialog for game options.
    void    newCube		// Construct a new cube and display it.
	    (int xDim, int yDim, int zDim, int shMoves);
    void    shuffleCube ();	// Generate shuffling moves.
    int     pickANumber		// Pick a number at random,
	    (int lo, int hi);	//     in the range (lo..hi).

    void    appendMove ();	// Add a player's move to the list.

    void    startUndo (QString code, QString header);
    void    startRedo (QString code, QString header);

/******************************************************************************/
/********************** METHODS TO SUPPORT ANIMATION  *************************/
/******************************************************************************/

    void    startAnimation (QString dSeq, int sID, bool vShuffle, bool vMoves);

    void    startNextDisplay(); // Start executing a step from displaySequence.
    void    startDemo();	// Start the demo sequence.
    void    randomDemo();	// Generate a random demo.
    void    stopDemo();		// Stop the demo sequence.
    void    chooseMousePointer(); // Set cross or stopwatch (wait) cursor.
    bool    tooBusy();		// Blocks menus, keyboard and mouse during
				// animated displays of moves, etc.

    void    startBlinking ();	// Blink a slice that has been selected to move.

    void    startMoves		// Start showing a sequence of moves.
	    (int nMoves, int index, bool pUndo, bool animation);
    void    startAnimatedMove	// Start showing a slice of the cube in motion.
            (Move * move, bool animation);
    void    startNextMove	// Initiate the next move in a sequence.
	    (bool animation);

    void    tumble();		// Tumble the cubes around if tumbling = TRUE.

/******************************************************************************/
/********************** METHODS TO SUPPORT THE MOUSE  *************************/
/******************************************************************************/

    // Evaluate and check a slice-move selected with the mouse.  Return 1 = OK.
    int     evaluateMove (bool found, int face []);
    void    showMouseMoveProgress ();
    int     findWhichCube (int mX, int mY);
};

class Cubie;		// Forward declaration of RubikCube's component.

// Face-sticker colors are in the order of axes X/Y/Z then -ve/+ve direction.
// INTERNAL is the color of the material of which the cube is made (eg. gray).
enum	FaceColor	{INTERNAL, LEFT, RIGHT, BOTTOM, TOP, BACK, FRONT};

/** 
 * The RubikCube class represents a Rubik's cube in an abstract way.
 *
 * Actually the "cube" is a rectangular parallelepiped, which can have unequal
 * sides, like a brick, or can be one layer thick, like a mat.  The cube is
 * made up of many "cubies" (small cubes) stacked in a 3-D array.  The original
 * Rubik's cube was an array of 3x3x3 cubies, but in this version the sides can
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
class RubikCube : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor for the RubikCube object
     * @param parent	The parent widget
     * @param xlen	The number of cubies in the X direction (left to right)
     * @param ylen	The number of cubies in the Y direction (bottom to top)
     * @param zlen	The number of cubies in the Z direction (back to front)
     */
    RubikCube (QWidget* parent = 0, int xlen = 3, int ylen = 3, int zlen = 3);
    ~RubikCube();

    void drawCube    (GameGLView * gameGLView, float cubieSize, int moveAngle);
    void moveSlice   (Axis axis, int location, Rotation direction);
    bool findSticker (double position [nAxes], float myCubieSize,
				int faceCentre [nAxes]);

    void setNextMove       (Axis axis, int location);
    void setBlinkingOn     (Axis axis, int location);
    void setBlinkingOff    ();

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

#endif	// GAME_H
