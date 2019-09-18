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

#ifndef GAME_H
#define GAME_H

// The Cube object uses the sqrt() function.
#include <cmath>
#include <cstdlib>

// KDE includes
#include <KStandardGameAction>	// Used only to get internal names of actions.
#include <KRandomSequence>

// Qt includes
#include <QObject>
#include <QString>
#include <QList>
#include <QElapsedTimer>

// Local includes.
#include "kubrick.h"
#include "gameglview.h"
#include "gamedialog.h"
#include "kbkglobal.h"
#include "cube.h"

class Kubrick;
class GameView;		// Forward declaration of view.
class GameGLView;	// Forward declaration of OpenGL painter and view.
class MoveTracker;
class SceneLabel;

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
    explicit Game (Kubrick * parent = nullptr);

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
    void handleMouseEvent (MouseEvent event, int button, int mX, int mY);

public Q_SLOTS:
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
    void setStandardView	();	// Align to show R, F and U (top) faces.

    void redoAll                ();	// Redo all undone moves.

    void changeScene	(const int);	// Change the scene (View Menu).
    void cycleSceneUp		();	// Cycle up through the scenes.
    void cycleSceneDown		();	// Cycle down through the scenes.

    void watchShuffling		();	// Set/clear viewShuffle option.
    void watchMoves		();	// Set/clear viewMoves option.
    void enableMessages		();	// Re-enable all "don't show" messages.
    void optionsDialog		();	// Open the dialog box for game options.

    // IDW - Key K for switching the background (temporary) - FIX IT FOR KDE 4.2
    void switchBackground       ();

    // Input for XYZ keyboard moves.
    void setMoveAxis		(int i);
    void setMoveSlice		(int slice);
    void setMoveDirection	(int direction);

    // Input for Singmaster keyboard moves, coded as a finite state machine.
    void smInput		(const int smCode);

private:
    // Implementation of states and state changes for Singmaster moves.
    QString singmasterString;		// Moves that have been entered.
    int     smSelectionStart;		// Highlighting of last move executed.
    int     smSelectionLength;
    QString smTempString;		// A move that is being entered.

    int     smDotCount;
    enum    KeyboardStateCode
	    {WaitingForInput, SingmasterPrefixSeen, SingmasterFaceIDSeen};
    KeyboardStateCode keyboardState;

    // The Singmaster move the player is entering or has just entered.
    Axis    smMoveAxis;
    int     smMoveSlice;
    Rotation smMoveDirection;

    // Display or re-display the Singmaster Moves.
    void smShowSingmasterMoves();

    // Check for an incomplete Singmaster move when there is another move to do.
    bool smMoveToComplete();

    // Initialise or re-initialise the move-text parsing.
    void smInitInput();

    // Processing of states for Singmaster moves.
    void smWaitingForInput	(const SingmasterMove smCode);
    void smSingmasterPrefixSeen	(const SingmasterMove smCode);
    void smSingmasterFaceIDSeen	(const SingmasterMove smCode);

    // Actions for Singmaster moves.
    void saveSingmasterFaceID   (const SingmasterMove smCode);
    void executeSingmasterMove  (const SingmasterMove smCode);

    QString convertMoveToSingmaster (const Move * move);

private Q_SLOTS:
    /**
    * This slot implements a game tick.  It increases the game tick counter,
    * triggers the OpenGL rendering and manages sequences of animated moves.
    * It also performs moves that are not animated, all at once, depending
    * on the current settings of the "view" options.
    **/
    void advance                ();

    /**
    * This slot adds a player's move to the list.  It is invoked either after
    * keyboard input or by a newMove signal from the moveTracker object.
    **/
    void addPlayersMove         (Move * move);

    /**
    * This slot records the fact that the user has rotated the cube manually.
    **/
    void setCubeNotAligned      ();

private:
    Kubrick *    myParent;	// Game's parent widget.
    Kubrick *    mainWindow;	// Main window: used for status, etc.
    GameGLView * gameGLView;	// OpenGL view: used to draw 3D cubes.

    KRandomSequence random;	// Random number generator object.
    Cube *   cube;		// The cube that is in play.
    float    cubieSize;		// Size of each cubie in OpenGL co-ordinates.
    QList<CubeView *> cubeViews; // Parameters for views of 1-3 cubes.

    SceneLabel * demoL;		// Text to say "DEMO - Click anywhere ...".
    SceneLabel * frontVL;	// Text to say "Front View".
    SceneLabel * backVL;	// Text to say "Back View".

/******************************************************************************/
/**************** DATA THAT RECORDS THE STATE OF PLAY *************************/
/******************************************************************************/

    QString saveFilename;	// Last filename used for Save or SaveAs.
    int     currentSceneID;	// The set of CubeViews to be displayed.
    bool    tumbling;		// If true, cubes are tumbling around.
    bool    cubeAligned;	// If false, the user has rotated the cube.

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

    long    nTick;		// Game-tick counter.

    enum    Mover {None, Mouse, Keyboard};
    Mover   moveFeedback;	// If != None, player is selecting a move.

    int     movesToDo;		// Number of moves to animate in next sequence.
    bool    undoing;		// If true, reverse the direction of each move.
    int     pauseTicks;		// Counter for a pause during animation.

    int     moveAngle;		// Degrees turned so far in an animated move.
    int     moveAngleStep;	// Degrees to be turned per animation step.
    int     moveAngleMax;	// Total degrees to turn in an animated move.

    long    blinkStartTime;	// When to start showing feedback of a move.
    QElapsedTimer   time;

/******************************************************************************/
/***************************** MOUSE-CONTROL DATA *****************************/
/******************************************************************************/

    MoveTracker * moveTracker;	// An object to track the pointer during moves.

/******************************************************************************/
/********************* METHODS TO SUPPORT GAME ACTIONS ************************/
/******************************************************************************/

    // Save the current puzzle to a KDE config file.
    void    doSave     (bool getFilename);
    void    savePuzzle (KConfig & config);

    // Load a saved puzzle from a KDE config file.
    void    loadPuzzle (KConfig & config);

    void    setCubeView (int sceneID, bool rotates, float size,
		float relX, float relY,
		float turn, float tilt, int labelX, int labelY, LabelID label);

    void    setDefaults();	// Set default cube sizes and options.
    int     doOptionsDialog (bool changePuzzle); // Do dialog for game options.
    void    newCube		// Construct a new cube and display it.
	    (int xDim, int yDim, int zDim, int shMoves);
    void    shuffleCube ();	// Generate shuffling moves.
    int     pickANumber		// Pick a number at random,
	    (int lo, int hi);	//     in the range (lo..hi).

    void    startUndo (const QString &code, const QString &header);
    void    startRedo (const QString &code, const QString &header);

/******************************************************************************/
/********************** METHODS TO SUPPORT ANIMATION  *************************/
/******************************************************************************/

    void    appendMove (Move * move);
    void    forceImmediateMove (Axis axis, int slice, Rotation direction);
    void    forceImmediateMove (Move * move);
    void    truncateUndoneMoves(); // Delete all undone and not-redone moves.

    void    startAnimation (const QString &dSeq, int sID, bool vShuffle, bool vMoves);

    void    startNextDisplay(); // Start executing a step from displaySequence.
    void    startDemo();	// Start the demo sequence.
    void    randomDemo();	// Generate a random demo.
    void    stopDemo();		// Stop the demo sequence.
    void    chooseMousePointer(); // Set cross or stopwatch (wait) cursor.
    bool    tooBusy();		// Blocks menus, keyboard and mouse during
				// animated displays of moves, etc.

    void    startBlinking ();	// Blink a slice that has been selected to move.

    void    startMoves		// Start showing a sequence of moves.
	    (int nMoves, int index, bool pUndo, int speed);
    void    startAnimatedMove	// Start showing a slice of the cube in motion.
            (Move * move, int speed);
    void    startNextMove	// Initiate the next move in a sequence.
	    (int speed);

    void    tumble();		// Tumble the cubes around if tumbling = true.

/******************************************************************************/
/********************** METHODS TO SUPPORT THE MOUSE  *************************/
/******************************************************************************/

    // Evaluate and check a slice-move selected with the mouse.  Return 1 = OK.
    int     evaluateMove (bool found, int face []);
    void    showMouseMoveProgress ();
    int     findWhichCube (int mX, int mY);
};

#endif	// GAME_H
