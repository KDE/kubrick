/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Local includes
#include "game.h"
#include "movetracker.h"
#include "scenelabel.h"
#include "kubrick_debug.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardAction>

#include <QFileDialog>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

// Create the main game/document object
Game::Game (Kubrick * parent)
	: QObject           (parent),

	  smSelectionStart  (0),
	  smSelectionLength (0),

	  smMoveAxis        (Z),
	  smMoveSlice       (0),
	  smMoveDirection   (CLOCKWISE),

	  random(QDateTime::currentMSecsSinceEpoch()),
	  cubeAligned       (true),
          moveIndex         (-1)
{
    myParent = parent;
    cube = nullptr;
    gameGLView = nullptr;			// OpenGL view is not yet created.
    mainWindow = nullptr;			// MW exists, but the GUI is not set up.

    smInitInput();			// Initialise the move-text parsing.

    setDefaults ();			// Set all options to default values.
    restoreState ();			// Restore the last cube and its state.
    demoPhase = false;			// No demo yet.

    moveTracker = new MoveTracker (myParent);

    connect(moveTracker, &MoveTracker::newMove, this, &Game::addPlayersMove);
    connect(moveTracker, &MoveTracker::cubeRotated, this, &Game::setCubeNotAligned);

    blinkStartTime = 300;
}


Game::~Game ()
{
    delete frontVL;
    delete backVL;
    delete demoL;
    qDeleteAll(cubeViews);
    qDeleteAll(moves);
}


void Game::initGame (GameGLView * glv, Kubrick * mw)
{
    // OK, now we have an OpenGL view and all the GUI widgets in KXmlGuiWindow.
    gameGLView = glv;			// Save OpenGL view, used when drawing.
    mainWindow = mw;			// Save main window, shows status, etc.

    // mainWindow->setToggle (QStringLiteral("toggle_tumbling"), tumbling);
    mainWindow->setToggle (QStringLiteral("watch_shuffling"), (bool) option [optViewShuffle]);
    mainWindow->setToggle (QStringLiteral("watch_moves"),     (bool) option [optViewMoves]);
    gameGLView->setBevelAmount (option [optBevel]);
    gameGLView->setCursor (Qt::CrossCursor);

    // Create the view-label texts for cube pictures.
    frontVL = new SceneLabel (i18n("Front View"));
    backVL  = new SceneLabel (i18n("Back View"));

    // Don't show them at startup time.
    frontVL-> setVisible (false);
    backVL->  setVisible (false);

    // Create a label text for the demos.
    demoL  =  new SceneLabel (i18n("DEMO - Click anywhere to begin playing"));
    demoL->   setVisible (false);	// Show it whenever the demo starts.

    // Set the scene parameters for 1, 2 or 3 cubes: cube has ID, turnability,
    // size, position of centre, turn, tilt, label location and label widget.

    setCubeView (1, TURNS, 1.1,    0.0,   0.0, -45.0, +40.0, 0, 0, NoLabel);

    setCubeView (2, TURNS, 1.1,  -0.25,   0.0, -20.0, +40.0, 2, 0, FrontLbl);
    setCubeView (2, TURNS, 1.1,  +0.25,   0.0, 125.0, +30.0, 6, 0, BackLbl);

    setCubeView (3, TURNS, 1.1,  -0.17,   0.0, -30.0, +40.0, 0, 0, NoLabel);
    setCubeView (3, FIXED, 0.55, +0.33, +0.25, -60.0, +40.0, 7, 0, FrontLbl);
    setCubeView (3, FIXED, 0.55, +0.33, -0.25, 130.0, +40.0, 7, 4, BackLbl);

    // Create the first cube and picture here, otherwise paintGL() will
    // die horribly when the main window calls it during startup.

    saveState ();

    // This will set the scene, clear animation variables and do no more.
    currentSceneID = 0;  // Forces startAnimation() to initialise the scene.
    startAnimation (QString(), option [optSceneID], false, false);

    qDeleteAll(moves);
    moves.clear();
    shuffleMoves = 0;
    playerMoves = 0;

    startDemo ();			// Start the demo.
    randomDemo ();			// Main window has shown "Welcome".

    // Implement game ticks [SLOT(advance()) does most of the work in Kubrick].
    QTimer* timer = new QTimer (this);
    nTick             = 0;
    connect(timer, &QTimer::timeout, this, &Game::advance);

    timer->start (20);			// Tick interval is 20 msec.
}


/******************************************************************************/
/***************************** PUBLIC SLOTS ***********************************/
/******************************************************************************/

void Game::newPuzzle ()
{
    if (demoPhase) {
	toggleDemo();
    }
    else if (tooBusy()) {
	return;
    }
    if (shuffleMoves <= 0) {
	KMessageBox::information (myParent,
		i18n("Sorry, the cube cannot be shuffled at the moment.  The "
		     "number of shuffling moves is set to zero.  Please select "
		     "your number of shuffling moves in the options dialog at "
		     "menu item Game->Choose Puzzle Type->Make Your Own…"),
		i18nc("@title:window", "New Puzzle"));
    }

    // Create a new cube, with the same options as before, then shuffle it.
    newCube (cubeSize [X], cubeSize [Y], cubeSize [Z], shuffleMoves);
}


void Game::load ()
{
    if (demoPhase) {
	toggleDemo();
    }
    else if (tooBusy()) {
	return;
    }
    QString loadFilename = QFileDialog::getOpenFileName(myParent, i18nc("@title:window", "Load Puzzle"),
                                                        QString(), i18n("Kubrick Game Files (*.kbk)"));
    if (loadFilename.isNull()) {
	return;
    }

    KConfig config (loadFilename, KConfig::SimpleConfig);

    if (! config.hasGroup (QStringLiteral("KubrickGame"))) {
	KMessageBox::error(mainWindow,
                           i18n("The file '%1' is not a valid Kubrick game-file.", loadFilename));
	return;
    }

    loadPuzzle (config);
}


void Game::save ()
{
    doSave (false);			// Use previous file name, if available.
}


void Game::saveAs ()
{
    doSave (true);			// Use the file dialog to get a name.
}


void Game::changePuzzle (const Kubrick::PuzzleItem & puzzle)
{
    if (demoPhase) {
	toggleDemo();
    }
    else if (tooBusy()) {
	return;
    }
    // Set the options for the new puzzle-type.
    option [optXDim]         = puzzle.x;
    option [optYDim]         = puzzle.y;
    option [optZDim]         = puzzle.z;
    option [optShuffleMoves] = puzzle.shuffleMoves;
    option [optViewShuffle]  = puzzle.viewShuffle;
    option [optViewMoves]    = puzzle.viewMoves;
    mainWindow->setToggle (QStringLiteral("watch_shuffling"), (bool) option [optViewShuffle]);
    mainWindow->setToggle (QStringLiteral("watch_moves"),     (bool) option [optViewMoves]);

    // Build the new cube and shuffle it.
    newCube (option [optXDim], option [optYDim], option [optZDim],
			       option [optShuffleMoves]);
}


void Game::newCubeDialog ()
{
    if (demoPhase) {
	toggleDemo();
    }
    if (doOptionsDialog (true) == QDialog::Accepted) {
	newCube (option [optXDim], option [optYDim], option [optZDim],
				   option [optShuffleMoves]);
	mainWindow->describePuzzle (option [optXDim], option [optYDim],
				option [optZDim], option [optShuffleMoves]);
    }
}


void Game::undoMove ()
{
    startUndo (QStringLiteral("u"), i18n("Undo"));
}


void Game::redoMove ()
{
    startRedo (QStringLiteral("r"), i18n("Redo"));
}


void Game::solveCube ()
{
    if (tooBusy())
	return;
    if (shuffleMoves <= 0) {
	KMessageBox::information (myParent,
		i18n("This cube has not been shuffled, so there is "
		     "nothing to solve."),
		i18nc("@title:window", "Solve the Cube"));
	return;
    }

    if (smMoveToComplete()) {
	// There is an incomplete Singmaster move, so undo that first.
	smInitInput();
	smShowSingmasterMoves();	// Re-display the Singmaster moves.
    }

    if (playerMoves > 0) {
	// Undo player moves, wait, solve (undo shuffle), wait, redo shuffle.
	QString s;
    s = s.fill (QLatin1Char('r'), playerMoves);	// Afterwards, redo each player move.
    startAnimation (QStringLiteral("Uwwswwhww") + s, option [optSceneID], true, false);
    }
    else {
	// No player moves: solve (undo shuffle), wait, redo shuffle.
    startAnimation (QStringLiteral("swwh"), option [optSceneID], true, false);
    }
}


void Game::setCubeNotAligned()
{
    cubeAligned = false;		// User has rotated the cube manually.
}


void Game::setStandardView()
{
    if (tooBusy())
	return;

    if (cubeAligned) {
	return;				// The cube is already aligned.
    }

    QList<Move *> tempMoves;
    moveTracker->realignCube (tempMoves);
    cubeAligned = true;

    if (tempMoves.isEmpty()) {
	return;				// The cube was close to being aligned.
    }

    // Transfer all the whole-cube alignment moves into the player's list and
    // execute them.  They can then be undone/redone.  More importantly, the
    // cube's internal axes will be aligned with the player's eye view, making
    // keyboard (XYZ) and Singmaster (LRFBUD) moves properly meaningful.

    // Delete moves that have been undone and not redone.
    truncateUndoneMoves();

    // Make sure the latest Singmaster move is completed, if there is one.
    if (smMoveToComplete()) {
	// Record and make the move internally, but do not animate it.  This is
	// a rare case but must be handled this way, because the cube moves that
	// will come after the Singmaster move cannot be animated.
	forceImmediateMove (smMoveAxis, smMoveSlice, smMoveDirection);

	// Add it to the Singmaster Moves display.
	singmasterString.append (smTempString);

	smInitInput();			// Re-initialise the move-text parsing.
    }

    // Record and make each whole-cube move internally, but do not animate it.
    // The resulting view of the cube has already been adjusted and displayed.
    // The series of moves is delimited by spaces in the Singmaster display.

    if ((singmasterString.length() > 0) &&
	(singmasterString.right (1) != QLatin1Char(SingmasterNotation [SM_SPACER])))
    {
	singmasterString.append (QLatin1Char(SingmasterNotation [SM_SPACER]));
    }

    while (! tempMoves.isEmpty()) {
	Move * move = tempMoves.takeFirst();
	forceImmediateMove (move);

	QString s         = convertMoveToSingmaster (move);
	smSelectionStart  = singmasterString.length();
	smSelectionLength = s.length();
	singmasterString.append (s);
    }

    singmasterString.append (QLatin1Char(SingmasterNotation [SM_SPACER]));
    smSelectionLength++;

    // Update the Singmaster Moves display.
    smShowSingmasterMoves();
}


void Game::undoAll ()
{
    startUndo (QStringLiteral("U"), i18n("Restart Puzzle (Undo All)"));
}


void Game::redoAll ()
{
    startRedo (QStringLiteral("R"), i18n("Redo All"));
}


void Game::changeScene (const int newSceneID)
{
    QString sceneActionName = QStringLiteral("scene_%1").arg(newSceneID);
    mainWindow->setToggle (sceneActionName, true);

    currentSceneID = newSceneID;
    option [optSceneID] = currentSceneID;
    setSceneLabels ();
}


void Game::cycleSceneUp ()
{
    // Add a cube to the view or cycle forward to a 1-cube view.
    changeScene ((currentSceneID < (nSceneIDs - 1)) ? (currentSceneID + 1)
						  : OneCube);
}


void Game::cycleSceneDown ()
{
    // Remove a cube from the view or cycle back to a 3-cube view.
    changeScene ((currentSceneID > 1) ? (currentSceneID - 1) : ThreeCubes);
}


void Game::toggleDemo ()
{
    if (demoPhase) {
	stopDemo ();
	restoreState ();
	mainWindow->describePuzzle (option [optXDim], option [optYDim],
				option [optZDim], option [optShuffleMoves]);
    }
    else {
	saveState ();
	startDemo ();
	randomDemo ();
    }
}


void Game::loadDemo (const QString & file)
{
    if ((! demoPhase) && tooBusy())
	return;
    QString demoFile = QStandardPaths::locate(QStandardPaths::AppDataLocation, file);
    KConfig config (demoFile, KConfig::SimpleConfig);
    if (config.hasGroup (QStringLiteral("KubrickGame"))) {
	if (! demoPhase) {
	    saveState ();
	    startDemo ();
	}
	loadPuzzle (config);
    }
    else {
	KMessageBox::information (myParent,
		i18n("Sorry, could not find a valid Kubrick demo file "
		     "called %1.  It should have been installed in the "
		     "'apps/kubrick' sub-directory.", file),
		i18nc("@title:window", "File Not Found"));
    }
}


void Game::watchShuffling ()
{
    bool v;
    v = (bool) option [optViewShuffle];
    option [optViewShuffle] = (int) (! v);
    if (! demoPhase) {
	viewShuffle = (! v);
    }
}


void Game::watchMoves ()
{
    bool v;
    v = (bool) option [optViewMoves];
    option [optViewMoves] = (int) (! v);
    if (! demoPhase) {
	viewMoves = (! v);
    }
}


void Game::enableMessages ()
{
    // Show error messages that have been disabled by the "don't show" option.
    KMessageBox::enableAllMessages ();
}


void Game::optionsDialog ()
{
    (void) doOptionsDialog (false);
}


// IDW - Key K for switching the background (temporary) - FIX IT FOR KDE 4.2.
void Game::switchBackground()
{
    gameGLView->changeBackground ();
}


void Game::setMoveAxis (int i)
{
    if (tooBusy())
	return;

    if (! cubeAligned) {
	setStandardView();		// Make sure the cube is realigned.
    }

    // Triggered by key x, y or z.
    currentMoveAxis = (Axis) i;
    startBlinking ();
}


void Game::setMoveSlice (int slice)
{
    if (tooBusy())
	return;

    if (! cubeAligned) {
	setStandardView();		// Make sure the cube is realigned.
    }

    // Triggered by keys 1 to 6 or C for rotating whole cube.
    if (slice > cubeSize [currentMoveAxis])
	return;				// Invalid (for current cube).
    if (slice == 0) {
	// Rotate the whole cube.
	currentMoveSlice = WHOLE_CUBE;
    }
    else {
	// Calculate the Cube co-ordinate of the slice to rotate.
	currentMoveSlice = 2 * slice - cubeSize [currentMoveAxis] - 1;
    }
    startBlinking ();
}


void Game::setMoveDirection (int direction)
{
    if (tooBusy())
	return;

    if (! cubeAligned) {
	setStandardView();		// Make sure the cube is realigned.
    }

    // Triggered by LeftArrow or RightArrow key.
    currentMoveDirection = (Rotation) direction;
    cube->setBlinkingOff ();
    moveFeedback = None;

    Move * move          = new Move;

    move->axis           = currentMoveAxis;
    move->slice          = currentMoveSlice;
    move->direction      = currentMoveDirection;

    addPlayersMove (move);		// Add the move to the list.
}


void Game::addPlayersMove (Move * move)
{
    // Mouse moves and XYZ moves come here. Singmaster moves do not.
    // So check if there is a Singmaster move to be completed.

    if (smMoveToComplete()) {
	// Complete the Singmaster move and add it to the list of moves to do.
	// This also deletes moves that have been undone and not redone.
	smInput (SM_EXECUTE);
    }
    else {
	// Delete moves that have been undone and not redone.
	truncateUndoneMoves();
    }

    // Add the new move to the queue.
    appendMove (move);

    // Add the move to the Singmaster Moves display.
    QString tempString = convertMoveToSingmaster (move);
    singmasterString.append (tempString);

    // Trigger the animation's advance() cycle to do the move(s).
    startAnimation (displaySequence + QLatin1Char('m'), option [optSceneID],
			option [optViewShuffle], option [optViewMoves]);
}


void Game::smShowSingmasterMoves()
{
    // Display or re-display the Singmaster moves, if the GUI has been set up.
    if (mainWindow != nullptr) {
	mainWindow->setSingmaster (singmasterString + smTempString);
	mainWindow->setSingmasterSelection
			(smSelectionStart, smSelectionLength);
    }
}


bool Game::smMoveToComplete()
{
    if (!smTempString.isEmpty()) {
	if (keyboardState == SingmasterFaceIDSeen) {
	    // Singmaster move must stay in list when next move is added.
	    return true;
	}
	else {
	    // Incomplete move: prefix only.  Re-initialise move-text parsing.
	    smInitInput();
	}
    }
    return false;			// No Singmaster move to complete.
}


void Game::smInitInput()
{
    // Initialise or re-initialise the move-text parsing.
    smDotCount = 0;
    smTempString.clear();
    keyboardState = WaitingForInput;
}


void Game::smInput (const int smCode)
{
    if (tooBusy())
	return;

    if (! cubeAligned) {
	setStandardView();		// Make sure the cube is realigned.
    }

// States: WaitingForInput, SingmasterPrefixSeen, SingmasterFaceIDSeen
//
// STATE                 INPUT      ACTIONS                NEXT-STATE
//
// WaitingForInput       SM_INNER   Count, limit           SingmasterPrefixSeen
//                       SM_A_CLOCK Error                  No change
//                       SM_DOUBLE  Error                   "   "
//                       SM_2_SLICE Error                   "   "
//                       SM_A_SLICE Error                   "   "
//                       SM_EXECUTE Error                   "   "
//                       SM_SPACER  Add space               "   "
//                       FaceID     Save faceID            SingmasterFaceIDSeen
// SingmasterPrefixSeen  SM_INNER   Count, limit           No change
//                       SM_A_CLOCK Error                   "   "
//                       SM_DOUBLE  Error                   "   "
//                       SM_2_SLICE Error                   "   "
//                       SM_A_SLICE Error                   "   "
//                       SM_EXECUTE Error                   "   "
//                       SM_SPACER  Error                   "   "
//                       FaceID     Save faceID            SingmasterFaceIDSeen
// SingmasterFaceIDSeen  SM_INNER   Execute, count, limit  SingmasterPrefixSeen
//                       SM_A_CLOCK Execute                WaitingForInput
//                       SM_DOUBLE  Execute                 "   "
//                       SM_2_SLICE Execute                 "   "
//                       SM_A_SLICE Execute                 "   "
//                       SM_EXECUTE Execute                 "   "
//                       SM_SPACER  Execute, add space      "   "
//                       FaceID     Execute, save faceID   No change

    switch (keyboardState) {
    case WaitingForInput:
	smInitInput();			// Playing safe.
	smWaitingForInput      ((SingmasterMove) smCode);
	break;
    case SingmasterPrefixSeen:
	smSingmasterPrefixSeen ((SingmasterMove) smCode);
	break;
    case SingmasterFaceIDSeen:
	smSingmasterFaceIDSeen ((SingmasterMove) smCode);
	break;
    default:
	break;
    }
    smShowSingmasterMoves();		// Re-display the Singmaster moves.
}


void Game::smWaitingForInput (const SingmasterMove smCode)
{
    switch (smCode) {
    case SM_INNER:
	smDotCount++;
	smTempString.append (QLatin1Char(SingmasterNotation [SM_INNER]));
	keyboardState = SingmasterPrefixSeen;	// Change the state.
	break;
    case SM_ANTICLOCKWISE:
    case SM_DOUBLE:
    case SM_2_SLICE:
    case SM_ANTISLICE:
    case SM_EXECUTE:
	// USER'S TYPO: Swallow the keystroke and do not change the state.
	keyboardState = WaitingForInput;
	break;
    case SM_SPACER:
	singmasterString.append (QLatin1Char(SingmasterNotation [SM_SPACER]));
	keyboardState = WaitingForInput;	// No change of state.
	break;
    case SM_UP:
    case SM_DOWN:
    case SM_LEFT:
    case SM_RIGHT:
    case SM_FRONT:
    case SM_BACK:
	saveSingmasterFaceID (smCode);
	keyboardState = SingmasterFaceIDSeen;	// Change the state.
	break;
    default:
	qCDebug(KUBRICK_LOG) << "Unknown Singmaster code" << smCode;
	break;
    }
}


void Game::smSingmasterPrefixSeen (const SingmasterMove smCode)
{
    switch (smCode) {
    case SM_INNER:
	smDotCount++;
	smTempString.append (QLatin1Char(SingmasterNotation [SM_INNER]));
	keyboardState = SingmasterPrefixSeen;	// No change of state.
	break;
    case SM_ANTICLOCKWISE:
    case SM_DOUBLE:
    case SM_2_SLICE:
    case SM_ANTISLICE:
    case SM_EXECUTE:
    case SM_SPACER:
	// USER'S TYPO: Swallow the keystroke and do not change the state.
	keyboardState = SingmasterPrefixSeen;
	break;
    case SM_UP:
    case SM_DOWN:
    case SM_LEFT:
    case SM_RIGHT:
    case SM_FRONT:
    case SM_BACK:
	saveSingmasterFaceID (smCode);
	keyboardState = SingmasterFaceIDSeen;	// Change the state.
	break;
    default:
	qCDebug(KUBRICK_LOG) << "Unknown Singmaster code" << smCode;
	break;
    }
}


void Game::smSingmasterFaceIDSeen (const SingmasterMove smCode)
{
    switch (smCode) {
    case SM_INNER:
	executeSingmasterMove (SM_EXECUTE);
	smDotCount = 1;
	smTempString = QLatin1Char(SingmasterNotation [SM_INNER]);
	keyboardState = SingmasterPrefixSeen;	// Change the state.
	break;
    case SM_ANTICLOCKWISE:
    case SM_DOUBLE:
    case SM_2_SLICE:
    case SM_ANTISLICE:
    case SM_EXECUTE:
	executeSingmasterMove (smCode);
	keyboardState = WaitingForInput;	// Change the state.
	break;
    case SM_SPACER:
	executeSingmasterMove (smCode);
	keyboardState = WaitingForInput;	// Change the state.
	break;
    case SM_UP:
    case SM_DOWN:
    case SM_LEFT:
    case SM_RIGHT:
    case SM_FRONT:
    case SM_BACK:
	executeSingmasterMove (SM_EXECUTE);
	saveSingmasterFaceID (smCode);
	keyboardState = SingmasterFaceIDSeen;	// No change of state.
	break;
    default:
	qCDebug(KUBRICK_LOG) << "Unknown Singmaster code" << smCode;
	break;
    }
}


void Game::saveSingmasterFaceID (const SingmasterMove smCode)
{
    int     direction;
    int     slice;

    switch (smCode) {
    case SM_UP:
	smMoveAxis = (Axis) Y;
	direction = +1;			// Face visible.
	break;
    case SM_DOWN:
	smMoveAxis = (Axis) Y;
	direction = -1;			// Face not visible.
	break;
    case SM_RIGHT:
	smMoveAxis = (Axis) X;
	direction = +1;			// Face visible.
	break;
    case SM_LEFT:
	smMoveAxis = (Axis) X;
	direction = -1;			// Face not visible.
	break;
    case SM_FRONT:
	smMoveAxis = (Axis) Z;
	direction = +1;			// Face visible.
	break;
    case SM_BACK:
	smMoveAxis = (Axis) Z;
	direction = -1;			// Face not visible.
	break;
    default:
	qCDebug(KUBRICK_LOG) << "'Impossible' Singmaster code" << smCode;
	return;
	break;
    }

    // We can only have inner slices on cube dimensions greater than 2 slices
    // and the number of inner slices is 2 less than the cube dimension, so
    // adjust the dot-count if necessary.

    smDotCount   = (cubeSize [smMoveAxis] <= 2) ? 0 : smDotCount;
    smDotCount   = (smDotCount > cubeSize [smMoveAxis] - 2) ?
			(cubeSize [smMoveAxis] - 2) : smDotCount;

    // Edit the temporary Singmaster move-string and add the move-notation.
    smTempString = (smDotCount > 0) ? smTempString.left (smDotCount) : QString();
    smTempString.append (QLatin1Char(SingmasterNotation [smCode]));

    // An invisible face (D, L, B) will be slice 1 and a visible face will have
    // a slice number the same as the cube dimension.  This is adjusted up or
    // down by the dot-count, so as to be that many slices "in" from the face.

    slice = (direction < 0) ? 1 : cubeSize [smMoveAxis];
    slice = slice - direction * smDotCount;

    // Calculate the Cube co-ordinate of the slice to rotate.

    smMoveSlice = 2 * slice - cubeSize [smMoveAxis] - 1;

    // In Singmaster convention, faces rotate clockwise, as seen when looking
    // at the face and towards the centre of the cube, so the invisible faces
    // must rotate ANTI-clockwise in the Kubrick convention, as seen from the
    // Up, Front, Right perspective view, looking at faces U, F and R.

    smMoveDirection = (direction < 0) ? ANTICLOCKWISE : CLOCKWISE;

    // NOTE: The move can be further modified if ' 2 + or - is appended.
}


void Game::executeSingmasterMove (const SingmasterMove smCode)
{
    switch (smCode) {
    case SM_ANTICLOCKWISE:
	smTempString.append (QLatin1Char(SingmasterNotation [SM_ANTICLOCKWISE]));
	smMoveDirection = (smMoveDirection == CLOCKWISE) ?
				ANTICLOCKWISE : CLOCKWISE;
	break;
    case SM_DOUBLE:
	smTempString.append (QLatin1Char(SingmasterNotation [SM_DOUBLE]));
	smMoveDirection = ONE_EIGHTY;
	break;
    case SM_2_SLICE:
    case SM_ANTISLICE:
    case SM_EXECUTE:
	break;
    case SM_SPACER:
	smTempString.append (QLatin1Char(SingmasterNotation [SM_SPACER]));
	break;
    default:
	qCDebug(KUBRICK_LOG) << "'Impossible' Singmaster code" << smCode;
	return;
	break;
    }

    Move * move          = new Move;

    move->axis           = smMoveAxis;
    move->slice          = smMoveSlice;
    move->direction      = smMoveDirection;

    // Delete moves that have been undone and not redone.
    truncateUndoneMoves();

    // Add the new move to the queue.
    appendMove (move);

    // Add it to the Singmaster Moves display.
    singmasterString.append (smTempString);

    // Trigger the animation's advance() cycle to do the move.
    startAnimation (displaySequence + QLatin1Char('m'), option [optSceneID],
			option [optViewShuffle], option [optViewMoves]);

    smInitInput();			// Re-initialise the move-text parsing.
}


QString Game::convertMoveToSingmaster (const Move * move)
{
    SingmasterMove s;
    switch ((int) move->axis) {
    case X:
	s = (move->slice < 0) ? SM_LEFT : SM_RIGHT;
	break;
    case Y:
	s = (move->slice < 0) ? SM_DOWN : SM_UP;
	break;
    case Z:
	s = (move->slice < 0) ? SM_BACK : SM_FRONT;
	break;
    }

    QString dots;
    int     slice = (move->slice + cubeSize [move->axis] + 1) / 2;
    int     direction = move->direction;

    if (move->slice == WHOLE_CUBE) {
	dots = QLatin1Char(SingmasterNotation [SM_CUBE]);
    }
    else {
	dots.fill (QLatin1Char(SingmasterNotation [SM_INNER]), cubeSize [move->axis]);
	dots = (move->slice < 0) ? dots.left (slice - 1) :
			       dots.left (cubeSize [move->axis] - slice);
    }

    if (move->slice < 0) {
	direction = (direction == CLOCKWISE) ? ANTICLOCKWISE : CLOCKWISE;
    }

    QString smMove = dots + QLatin1Char(SingmasterNotation [s]) + ((direction == CLOCKWISE) ?
            QString() : QString (QLatin1Char(SingmasterNotation [SM_ANTICLOCKWISE])));
    return smMove;
}


void Game::setSceneLabels ()
{
    int		x, y;
    int		w = gameGLView->width();
    int		h = gameGLView->height();
    SceneLabel * labelObj = nullptr;

    frontVL->setVisible (false);
    backVL->setVisible  (false);

    for (CubeView * v : std::as_const(cubeViews)) {
	if ((v->sceneID != currentSceneID) || (v->label == NoLabel))
	    continue;			// Skip unwanted scene IDs and labels.

	// Convert label-index to run-time label object pointer.
	switch ((int) v->label) {
	case FrontLbl:
	    labelObj = frontVL;
	    break;
	case BackLbl:
	    labelObj = backVL;
	    break;
	}

	// Position the label in 1/8ths of gameGLView dimensions.
	x = (v->labelX * w)/8 - labelObj->width()/2 + 10;
	y = (v->labelY * h)/8 + labelObj->height();
	labelObj->move (x, y);
	labelObj->setVisible (true);
    }
    demoL->move (10, gameGLView->height() - 10);
}


void Game::saveState ()
{
    if (demoPhase) {
	return;				// Don't save if quitting during a demo.
    }
    const QString sFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + QStringLiteral("kubrick.save");
    KConfig config (sFile, KConfig::SimpleConfig);
    savePuzzle (config);
}


void Game::restoreState ()
{
    const QString rFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + QStringLiteral("kubrick.save");
    KConfig config (rFile, KConfig::SimpleConfig);
    if (config.hasGroup (QStringLiteral("KubrickGame"))) {
	loadPuzzle (config);
    }
    else {
	// No Kubrick game-file in this user's area, so save a default puzzle.
	newCube (option [optXDim], option [optYDim], option [optZDim],
						     option [optShuffleMoves]);
	savePuzzle (config);
    }
}


void Game::newCube (int xDim, int yDim, int zDim, int shMoves)
{
    delete cube;			// Delete the previous cube (if any).
    cubeSize [X] = xDim;
    cubeSize [Y] = yDim;
    cubeSize [Z] = zDim;
    nMax = qMax(qMax(xDim, yDim), zDim);
    shuffleMoves = shMoves;

    cube = new Cube (this, cubeSize[X], cubeSize[Y], cubeSize[Z]);

    // Set default parameters for the first move (if using keyboard control).
    currentMoveAxis = Z;
    currentMoveSlice = cubeSize[Z] - 1;	// Front face (+Z).
    currentMoveDirection = CLOCKWISE;

    moveFeedback = None;		// No move being selected.

    qDeleteAll(moves);			// Re-initialise the internal move-list.
    moves.clear();
    playerMoves = 0;

    singmasterString = QString();		// Re-initialise the Singmaster moves
    smSelectionStart = 0;		// display and move-text parsing.
    smSelectionLength = 0;
    smInitInput();

    // Clear the Singmaster Moves display.
    smShowSingmasterMoves();

    // Shuffle the cube.
    QString dSeq;			// No moves to do, if no shuffling.
    if (shuffleMoves > 0) {
	shuffleCube ();			// Calculate the shuffling moves.
    dSeq = QLatin1Char('h');		// Ask to do the shuffling moves.
    }
    // Trigger the animation's advance() cycle to do the moves.
    startAnimation (dSeq, option [optSceneID], option [optViewShuffle],
					       option [optViewMoves]);
}


int Game::doOptionsDialog (bool changePuzzle) // Private function.
{
    int result = QDialog::Rejected;
    if (tooBusy())
	return (result);
    int optionTemp [nOptions];

    LOOP (n, nOptions) {
	optionTemp [n] = option [n];
    }

    // Show the options dialog for the new cube.
    GameDialog * d = new GameDialog (changePuzzle, optionTemp, myParent);

    int count = 0;
    result = QDialog::Accepted;
    while ((result = d->exec()) == QDialog::Accepted) {
	// The number of cubies on each edge is >=1 and <= 6.
	count = 0;
	LOOP (axis, nAxes) {
	    if (optionTemp [optXDim + axis] == 1) {
		count++;
	    }
	}

	// At most one side should have size 1.
	if (count > 1) {
	    KMessageBox::information (myParent,
		    i18n("Only one of your dimensions can be one cubie wide."),
		    i18nc("@title:window", "Cube Options"));
	    continue;			// Repeat the dialog.
	}
	else {
	    break;			// The dimensions are valid.
	}
    }

    if (result == QDialog::Accepted) {
	LOOP (n, nOptions) {
	    option [n] = optionTemp [n];
	}
	moveSpeed = option [optMoveSpeed];
	gameGLView->setBevelAmount (option [optBevel]);
	viewShuffle = (bool) option [optViewShuffle];
	viewMoves = (bool) option [optViewMoves];
	mainWindow->setToggle (QStringLiteral("watch_shuffling"), (bool)option[optViewShuffle]);
	mainWindow->setToggle (QStringLiteral("watch_moves"),     (bool)option[optViewMoves]);
    }

    delete d;

    return (result);
}


void Game::drawScene ()
{
    // Save data with QOpenGLWidget only valid during paintGL()
    moveTracker->saveSceneInfo();

    // The size of the cubes on-screen is determined by the height of the
    // window and the OpenGL perspective projection, in which the window-height
    // always has a set angle of view.  The lateral spacing of the cubes (X
    // co-ordinates of centres) is scaled by the aspect-ratio, to give us an
    // even spacing however wide the window is.

    double aspect = (double) gameGLView->width () / gameGLView->height ();
    float fieldHeight = -cubeCentreZ * 2.0 * tan (3.14159 * viewAngle/360.0);
    float fieldWidth  = aspect * fieldHeight;

    for (CubeView * v : std::as_const(cubeViews)) {
	if (v->sceneID != currentSceneID)
	    continue;			// Skip unwanted scene IDs.

	v->position [X] = v->relX * fieldWidth;
	v->position [Y] = v->relY * fieldHeight;
	v->position [Z] = cubeCentreZ;

	gameGLView->pushGLMatrix ();
	gameGLView->moveGLView (v->position[X],
				v->position[Y],
				v->position[Z]);
	v->cubieSize = v->size / nMax;

	// Turn and tilt, to make 3 faces visible.
	gameGLView->rotateGLView (v->turn, 0.0, 1.0, 0.0);
	gameGLView->rotateGLView (v->tilt, 1.0, 0.0, -1.0);

	// Save the matrix for this (standard) view of the cube.
	glGetDoublev (GL_MODELVIEW_MATRIX, v->matrix0);

	// Tumble or rotate the cube as required, if this cube-view can rotate.
	if (v->rotates) {
	    if (demoPhase) {
		// Calculate a pseudo-random rotation.
		tumble();
	    }
	    else {
		// Apply the total of the user's arbitrary rotations (if any).
		moveTracker->usersRotation();
	    }
	}

	// Save the matrix for this (fully rotated) view of the cube.
	glGetDoublev (GL_MODELVIEW_MATRIX, v->matrix);

	cube->drawCube (gameGLView, v->cubieSize);

	gameGLView->popGLMatrix ();
    }

    // Draw whichever scene-labels are visible.
    demoL->   drawLabel (gameGLView);	// "DEMO - Click anywhere ...".
    frontVL-> drawLabel (gameGLView);	// "Front View".
    backVL->  drawLabel (gameGLView);	// "Back View".
}


void Game::setCubeView (int sceneID, bool rotates, float size,
		float relX, float relY,
		float turn, float tilt, int labelX, int labelY, LabelID label)
{
    CubeView * v = new CubeView;

    double aspect = (double) gameGLView->width () / gameGLView->height ();
    float fieldHeight = -cubeCentreZ * 2.0 * tan (3.14159 * viewAngle/360.0);
    float fieldWidth  = aspect * fieldHeight;

    v->sceneID      = sceneID;
    v->rotates      = rotates;
    v->size         = size;
    v->relX         = relX;
    v->relY         = relY;
    v->position [X] = v->relX * fieldWidth;
    v->position [Y] = v->relY * fieldHeight;
    v->position [Z] = cubeCentreZ;
    v->turn         = turn;
    v->tilt         = tilt;
    v->labelX       = labelX;
    v->labelY       = labelY;
    v->label        = label;

    cubeViews.append (v);
}


void Game::tumble ()
{
    if (tumblingTicks > 0) {
	// If the cube has been tumbled, restore its rotation position or
	// tumble it some more (advance() bumps tumblingTicks if tumbling on).
	gameGLView->rotateGLView ((float) (tumblingTicks % 360),
					1.0, 0.0, 0.0);		// X rotation.
	gameGLView->rotateGLView ((float) (((tumblingTicks * 9) / 10) % 360),
					0.0, 1.0, 0.0);		// Y rotation.
	gameGLView->rotateGLView ((float) (((tumblingTicks * 8) / 10) % 360),
					0.0, 0.0, 1.0);		// Z rotation.
    }
}


void Game::startDemo ()
{
    // Set the demo's toggle-button ON.
    mainWindow->setToggle (KGameStandardAction::name (KGameStandardAction::Demo), true);
    // Disable the Save actions.
    mainWindow->setAvail (KGameStandardAction::name (KGameStandardAction::Save),   false);
    mainWindow->setAvail (KGameStandardAction::name (KGameStandardAction::SaveAs), false);
    // Disable the Preferences action.
    mainWindow->setAvail (KStandardAction::name (KStandardAction::Preferences), false);

    demoPhase = true;
    tumblingTicks = 0;			// Show an untumbled cube.
    demoL->setVisible (true);		// Show the "click to stop" message.
}


void Game::randomDemo ()
{
    double pickShape = random.generateDouble();

    // Pick cubes 40% of the time.
    cubeSize [X] = pickANumber (2, 6);
    cubeSize [Y] = cubeSize [X];
    cubeSize [Z] = cubeSize [X];

    if (pickShape < 0.6) {
	// Pick square-cross-section prisms 40% of the time.
	while (cubeSize [Z] == cubeSize [X]) {
	    cubeSize [Z] = pickANumber (1, 6);
	}
    }

    if (pickShape < 0.2) {
	// Pick irregular shapes 20% of the time.
	while ((cubeSize [Y] == cubeSize [X]) ||
	       (cubeSize [Y] == cubeSize [Z])) {
	    cubeSize [Y] = pickANumber (2, 6);
	}
    }

    shuffleMoves      = pickANumber (5, 12);
    tumbling          = true;
    // mainWindow->setToggle ("toggle_tumbling", tumbling);
    moveSpeed         = 5;

    // Create the demo cube.
    newCube (cubeSize [X], cubeSize [Y], cubeSize [Z], shuffleMoves);

    // Shuffle, solve, start next demo ... current scene, all moves animated.
    startAnimation (QStringLiteral("whwswd"), currentSceneID, true, true);
    mainWindow->describePuzzle (cubeSize [X], cubeSize [Y], cubeSize [Z],
				shuffleMoves);
}


void Game::stopDemo ()
{
    // Set the demo's toggle-button OFF.
    mainWindow->setToggle (KGameStandardAction::name (KGameStandardAction::Demo), false);
    // Enable the Save actions.
    mainWindow->setAvail (KGameStandardAction::name (KGameStandardAction::Save),   true);
    mainWindow->setAvail (KGameStandardAction::name (KGameStandardAction::SaveAs), true);
    // Enable the Preferences action.
    mainWindow->setAvail (KStandardAction::name (KStandardAction::Preferences), true);

    tumbling = false;
    // mainWindow->setToggle (QStringLiteral("toggle_tumbling"), tumbling);

    demoL->setVisible (false);		// Hide the DEMO text.
    demoPhase = false;
}


void Game::setDefaults()
{
    // Set default values for the options.

    option [optXDim] = 3;		// Cube size 3x3x3.
    option [optYDim] = 3;
    option [optZDim] = 3;

    option [optShuffleMoves] = 4;	// Four shuffling moves.
    option [optViewShuffle] = (int) true; // Animate the shuffling moves.
    option [optViewMoves] = (int) false;  // Don't animate the player's moves.
    option [optBevel] = 12;		// 12% bevel on edges of cubies.
    option [optMoveSpeed] = 5;		// Speed of moves (5 deg/tick) [1..10].

    option [optSceneID] = TwoCubes;	// Scene: 2 cubes, front and back views.
    option [optTumbling] = (int) false;	// No tumbling.
    option [optTumblingTicks] = 0;	// "Home" orientation.
    option [optMouseBlink] = (int) true;  // Blink during mouse-controlled move.
}


void Game::startBlinking ()
{
    cube->setBlinkingOff ();
    cube->setBlinkingOn (currentMoveAxis, currentMoveSlice);
    moveFeedback = Keyboard;
    time.start();
}


void Game::appendMove (Move * move)
{
    move->degrees        = 90;

    // If single slice and not square, rotate 180 degrees rather than 90.
    if ((move->slice != WHOLE_CUBE) &&
	(cubeSize [(move->axis+1)%nAxes] != cubeSize [(move->axis+2)%nAxes])) {
	move->degrees = 180;
    }

    // IDW testing - qCDebug(KUBRICK_LOG) << move->axis << move->slice <<
			// IDW testing - move->direction << move->degrees;
    moves.append (move);
}


void Game::forceImmediateMove (Axis axis, int slice, Rotation direction)
{
    Move * move        = new Move;
    move->axis         = axis;
    move->slice        = slice;
    move->direction    = direction;
    forceImmediateMove (move);
}


void Game::forceImmediateMove (Move * move)
{
    // Add the move to the queue.
    appendMove (move);

    // Make the move internally and immediately, without animating it.
    playerMoves++;
    cube->moveSlice (move->axis, move->slice, move->direction);
}


void Game::truncateUndoneMoves()
{
    while (moves.count() > (shuffleMoves + playerMoves)) {
	delete moves.takeLast();	// Remove undone moves (if any).
    }
    singmasterString = singmasterString.left
			(smSelectionStart + smSelectionLength);
}


void Game::doSave (bool getFilename)
{
    if (demoPhase || tooBusy())
	return;
    if (saveFilename.isEmpty() || getFilename) {
	QString newFilename = QFileDialog::getSaveFileName(myParent, i18nc("@title:window", "Save Puzzle"),
                                                           QString(), i18n("Kubrick Game Files (*.kbk)"));
	if (newFilename.isNull()) {
	    return;
	}
	saveFilename = newFilename;
    }

    KConfig config (saveFilename, KConfig::SimpleConfig);
    savePuzzle (config);
}


void Game::savePuzzle (KConfig & config)
{
    // Make sure the latest Singmaster move is completed, if there is one.
    if (smMoveToComplete()) {
	// Record and make the move internally, but do not animate it.  This is
	// a rare case but should be handled this way, because the player may be
	// quitting, saving or switching to demo mode and should not lose input.

	// Delete moves that have been undone and not redone.
	truncateUndoneMoves();

	// Do the Singmaster move.
	forceImmediateMove (smMoveAxis, smMoveSlice, smMoveDirection);

	// Add it to the Singmaster Moves display.
	smSelectionStart  = singmasterString.length();
	smSelectionLength = smTempString.length();
	singmasterString.append (smTempString);

	smInitInput();			// Re-initialise the move-text parsing.

	smShowSingmasterMoves();	// Update the Singmaster Moves display.
    }

    // Clear any previously saved info (in case there are fewer moves now).
    config.deleteGroup (QStringLiteral("KubrickGame"));
    KConfigGroup configGroup = config.group(QStringLiteral("KubrickGame"));

    QStringList list;

    // Save the option settings.
    LOOP (i, nOptions) {
	list.append (QString::asprintf ("%d", option [i]));
    }
    configGroup.writeEntry ("a) Options", list);

    // Save the display sequence.
    configGroup.writeEntry ("c) DisplaySequence", displaySequence);
    QString dsTemp = configGroup.readEntry ("c) DisplaySequence", "");

    // Save the current move counts.
    list.clear ();
    list.append (QString::asprintf ("%d", shuffleMoves));
    list.append (QString::asprintf ("%d", playerMoves));
    list.append (QString::asprintf ("%" PRIdQSIZETYPE, moves.count()));
    configGroup.writeEntry ("f) MoveCounts", list);

    // Save the list of Singmaster moves.
    configGroup.writeEntry ("g) SingmasterMoves",   singmasterString);

    // Save the list of moves, using names "m) 001", "m) 002", etc.
    int n = 0;
    list.clear ();
    for (Move * m : std::as_const(moves)) {
	list.append (QString::asprintf ("%d", (int) m->axis));
	list.append (QString::asprintf ("%d", m->slice));
	list.append (QString::asprintf ("%d", (int) m->direction));
	list.append (QString::asprintf ("%d", m->degrees));

	n++;
	configGroup.writeEntry (QString::asprintf ("m) %03d", n), list);
	list.clear ();
    }

    configGroup.sync();			// Make sure it all goes to disk.
}


void Game::loadPuzzle (KConfig & config)
{
    QStringList list;
    QStringList notFound;
    QStringList::Iterator it;
    int optionTemp [nOptions];
    QString dsTemp;
    int moveCounts [3];
    QList<Move *> movesTemp;
    Move * moveTemp;

    KConfigGroup configGroup = config.group (QStringLiteral("KubrickGame"));

    list = configGroup.readEntry ("a) Options", notFound);
    int nOpt = 0;
    for (it = list.begin(); it != list.end(); ++it) {
	optionTemp [nOpt] = (*it).toInt();
	nOpt++;
    }

    dsTemp = configGroup.readEntry ("c) DisplaySequence", "");

    list = configGroup.readEntry ("f) MoveCounts", notFound);
    int n = 0;
    for (it = list.begin(); it != list.end(); ++it) {
	moveCounts [n] = (*it).toInt();
	n++;
    }

    QString key;
    for (n = 0; n < moveCounts [2]; ++n) {
	key = QString::asprintf ("m) %03d", n + 1);
	list = configGroup.readEntry (key, notFound);
	moveTemp = new Move;
	it = list.begin();
	moveTemp->axis      = (Axis) (*it).toInt();
	it++;
	moveTemp->slice     = (*it).toInt();
	it++;
	moveTemp->direction = (Rotation) (*it).toInt();
	it++;
	moveTemp->degrees   = (*it).toInt();
	movesTemp.append (moveTemp);
    }

    LOOP (n, nOpt) {
	option [n] = optionTemp [n];
    }

    // Build the new cube but don't shuffle it (yet).
    newCube (option [optXDim], option [optYDim], option [optZDim], 0);

    moveSpeed		= option [optMoveSpeed];
    if (gameGLView != nullptr) {
	gameGLView->setBevelAmount (option [optBevel]);
    }
    tumbling		= option [optTumbling];
    tumblingTicks	= option [optTumblingTicks];
    if (mainWindow != nullptr) {
	// mainWindow->setToggle (QStringLiteral("toggle_tumbling"), tumbling);
	mainWindow->setToggle (QStringLiteral("watch_shuffling"), (bool)option[optViewShuffle]);
	mainWindow->setToggle (QStringLiteral("watch_moves"),     (bool)option[optViewMoves]);
    }

    qDeleteAll(moves);
    moves               = movesTemp;
    shuffleMoves	= moveCounts [0];
    playerMoves		= moveCounts [1];

    // Restore the list of Singmaster moves.
    singmasterString = configGroup.readEntry  ("g) SingmasterMoves", "");

    // Show the Singmaster Moves, in case there are moves but all are undone.
    smShowSingmasterMoves();

    // If there are no saved moves and the cube should be shuffled, do it now.
    if ((moveCounts [2] == 0) && (option [optShuffleMoves] > 0)) {
	shuffleMoves = option [optShuffleMoves];
	shuffleCube ();
    }

    // Set the required display sequence, reconstructing it if necessary.
    QString dSeq = dsTemp;
    if (dSeq.isEmpty ()) {
	if (shuffleMoves > 0) {
        dSeq = QLatin1Char('h');
	}
	if (playerMoves > 0) {
	    // Redo all the player moves, using "M" (not "R", Redo All, in
	    // case there are some undone moves on the end of the list).
        dSeq += QLatin1Char('M');
	}
    }

    // If dSeq is empty, we will clear previous animations and do no more,
    // otherwise we will reconstruct the saved position by re-doing moves
    // and the Singmaster Moves display will also get updated if required.

    if (demoPhase) {
	// Always animate demo sequences (pretty patterns or solving moves).
	startAnimation (dSeq, option [optSceneID], option [optViewShuffle],
						   option [optViewMoves]);
    }
    else {
	// Never re-animate restoreState() or other saved user files.
	startAnimation (dSeq, option [optSceneID], false, false);
    }

    if (gameGLView != nullptr) {
	advance ();			// Do all moves or just one anim step.
	chooseMousePointer ();		// Choose busy or idle mouse pointer.
	setSceneLabels ();		// Position the labels.
    }
}


void Game::startUndo (const QString &code, const QString &header)
{
    if (tooBusy())
	return;

    if (smMoveToComplete()) {
	// There is an incomplete Singmaster move, so undo that first.
	smInitInput();
	smShowSingmasterMoves();	// Re-display the Singmaster moves.
    if ((playerMoves <= 0) || (code == QLatin1Char('u'))) {
	    return;			// The Undo or Undo All is finished.
	}
    }

    if (playerMoves <= 0) {
	KMessageBox::information (myParent,
		i18n("You have no moves to undo."),
		header);
	return;
    }

    // Start the undo off.
    startAnimation (code, option [optSceneID], option [optViewShuffle],
					       option [optViewMoves]);
}


void Game::startRedo (const QString &code, const QString &header)
{
    if (tooBusy())
	return;

    if (moves.count() > (shuffleMoves + playerMoves)) {
	// If there is an incompletely entered Singmaster move,
	// abandon it in favour of redoing older move(s).
	if (smMoveToComplete()) {
	    smInitInput();		// Undo the incomplete Singmaster input.
	    smShowSingmasterMoves();	// Re-display the Singmaster moves.
	}

	// Start the redo off.
	startAnimation (code, option [optSceneID], option [optViewShuffle],
						   option [optViewMoves]);
    }
    else {
	KMessageBox::information (myParent,
		i18n("There are no moves to redo.\n\nThat could be because "
		     "you have not undone any or you have redone them all or "
		     "because all previously undone moves are automatically "
		     "deleted whenever you make a new move using the keyboard "
		     "or mouse."),
		header);
    }
}

void Game::handleMouseEvent (MouseEvent event, int button, int mX, int mY)
{
    // A mouse click (press and release) stops the demo.
    if (demoPhase) {
	if (event == ButtonUp) {
	    stopDemo ();
	    restoreState ();
	    mainWindow->describePuzzle (option [optXDim], option [optYDim],
				option [optZDim], option [optShuffleMoves]);
	}
	return;
    }

    if (tooBusy())
	return;

    if (((event == ButtonDown) && (moveFeedback != None)) ||
	((event == ButtonUp) && (moveFeedback != Mouse))) {
	// There is move feedback on some other device (e.g. keyboard).
	return;			// Ignore mouse clicks.
    }

    // Start or end mouse feedback.
    if (event == ButtonDown) {
	moveFeedback = Mouse;
	time.start();
	gameGLView->setBlinkIntensity (1.0);	// Keep intensity high at first.
    }
    else {
	moveFeedback = None;
    }

    // Position in window follows OpenGL convention (with y = 0 at bottom).
    moveTracker->mouseInput (currentSceneID, cubeViews, cube,
			event, button, mX, mY);
}


int Game::pickANumber (int lo, int hi)
{
    // Pick an integer in the range (lo..hi).
    return random.bounded(lo, hi + 1);
}


void Game::shuffleCube ()
{
    Move * move;
    Move * prev;
    int    sliceNo;

    qDeleteAll(moves);
    moves.clear();

    LOOP (n, shuffleMoves) {
	move = new Move;

	while (true) {
	    // Calculate a random move.
	    move->axis      = (Axis)     pickANumber (0, nAxes - 1);
	    if (cubeSize [move->axis] == 1) {
		continue;	// Thin "cube" - don't rotate the whole thing.
	    }
	    sliceNo         = (int)      pickANumber (1, cubeSize [move->axis]);
	    move->slice     =            2*sliceNo - cubeSize [move->axis] - 1;
	    move->direction = (Rotation) pickANumber (ANTICLOCKWISE, CLOCKWISE);
	    move->degrees = 90;

	    // If the slice is not square, rotate 180 degrees rather than 90.
	    if (cubeSize [(move->axis+1)%nAxes] !=
		cubeSize [(move->axis+2)%nAxes]) {
		move->degrees = 180;
	    }

	    // Always accept the first move (n = 0).
	    if (n > 0) {
                prev = moves.at (n-1);		// Look at the last move.
		if ((move->axis  == prev->axis) &&
		    (move->slice == prev->slice) &&
		    ((move->degrees   == 180) ||
		     (move->direction != prev->direction))) {
		    // Reject a move that reverses the previous move.
		    continue;
		}
	    }
	    if (n > 1) {
		if (move->axis == prev->axis) {	// Axis same as in last move?
                    prev = moves.at (n-2);	// Look at the last but one.
		    if (move->axis == prev->axis) {
			// Reject a third successive move around the same axis.
			continue;
		    }
		}
	    }
	    break;
	}

	moves.append (move);
    }
}

void Game::startAnimation (const QString &dSeq, int sID, bool vShuffle, bool vMoves)
{
    // Set the scene ID, animation display sequence and whether to animate the
    // shuffle and/or player moves or do them instantly (within one tick).

    if ((sID != currentSceneID) && (mainWindow != nullptr)) {
        changeScene (sID);
	currentSceneID  = sID;
    }
    displaySequence = dSeq;
    viewShuffle     = vShuffle;
    viewMoves       = vMoves;

    // Initialise Game::advance().
    movesToDo       = 0;
    pauseTicks      = 0;
    undoing         = false;

    // Clear any previous animation.
    moveAngleMax  = 0;
    moveAngleStep = 0;
    moveAngle     = 0;

    // Game::advance() takes over and does the rest.
}


void Game::advance()
{
    // Increase the game tick counter
    //
    // Note that you might think this variable could overflow. However, even if
    // advance() was called once per ms (which would be far too often) it would
    // take at least 24 days of uninterrupted running until the variable would
    // overflow!  So usually you do not need to worry about it.

    nTick++;

    // This next counter keeps track of the position when the cube is tumbling.
    if (tumbling) {
	tumblingTicks++;			// Change tumbling position.
    }

    // If feedback to show player's next move, update it every 100 msec.
    if ((moveFeedback != None) && (nTick % 5 == 0)) {
	int t = time.elapsed();

	if (moveFeedback == Mouse) {
	    // Position in window uses OpenGL convention (y = 0 at bottom).
	    QPoint p = gameGLView->getMousePosition();
	    moveTracker->mouseInput (currentSceneID, cubeViews, cube,
			Tracking, 0, p.x(), p.y());
	}
	else {
	    // Blink slice(s) when selecting a move.
	    gameGLView->setBlinkIntensity ((t < blinkStartTime) ? 1.0 :
					((float) ((nTick/5)%3) * 0.1) + 0.5);
	}
    }


    // If moves to do, either animate the first one or do them all in one tick.
    while ((pauseTicks > 0) || (! displaySequence.isEmpty()) || (movesToDo > 0))
    {
	// If pausing, during an animation, count the time to pause.
	if (pauseTicks > 0) {
	    pauseTicks--;		// This is the wait sequence ('w').
	    break;
	}
	// Do any moves that are waiting to be done.
	if (movesToDo > 0) {
	    if (moveAngleMax > 0) {
		// The moves are being animated: change the move angle.
		moveAngle = moveAngle + moveAngleStep;
		if (abs(moveAngle) <= moveAngleMax) {
		    cube->setMoveAngle (moveAngle);
		    break;		// Let the animation continue.
		}
		// If that animated move is completed, start the next one.
		movesToDo--;
		cube->setMoveAngle (0);
		startNextMove (abs(moveAngleStep));
	    }
	    else {
		// Do all the moves at once, without any animation.
		movesToDo--;
		startNextMove (0);
	    }
	}
	// If there are no moves, see if there is a display item to be started.
	if (movesToDo <= 0) {
	    if (displaySequence.isEmpty()) {
		// The animation is finished.
		chooseMousePointer ();
	    }
	    else {
		// There is another animation sequence to do: start it now.
		startNextDisplay ();
	    }
	}
    } // End while

    // Force a redraw.  Note that we use a timer event to trigger it, so that
    // we do not do this in the advance() method itself.  This is not essential,
    // but makes the game-logic and rendering more independent of each other.

    QTimer::singleShot(0, gameGLView, qOverload<>(&GameGLView::update));
}


void Game::chooseMousePointer ()
{
    if (tumbling || (movesToDo > 0) || (! displaySequence.isEmpty())) {
	gameGLView->setCursor (Qt::WaitCursor);
    }
    else {
	gameGLView->setCursor (Qt::CrossCursor);
    }
}


void Game::startNextDisplay ()
{
    // Pick off the first character of the display sequence.
    char c = displaySequence.at(0).toLatin1();
    displaySequence.remove (0, 1);
    int nRMoves = 0;

    // Set the animation speed: 0 = no animation, 15 = fastest.  Note that if
    // the "Watch Your Own Moves" option is off, animation is set to very fast.
    int shSpeed = viewShuffle ? moveSpeed : 0;
    int mvSpeed = viewMoves   ? moveSpeed : defaultOwnMove;

    // Initiate the required sequence of moves (or single move).
    switch (c) {
    case 'd':			// Start a random demo sequence ("whwswd").
	randomDemo ();
	break;
    case 'h':			// Start a shuffling sequence of moves.
	startMoves (shuffleMoves, 0, false, shSpeed);
	break;
    case 'm':			// Start doing or
    case 'r':			// redoing a player's move.
	playerMoves++;
	startMoves (1, shuffleMoves + playerMoves - 1, false, mvSpeed);
	break;
    case 'M':			// Start restoring (reloading) a player's moves.
	mvSpeed = viewMoves ? moveSpeed : 0;	// Avoid an ugly fast animation.
	startMoves (playerMoves, shuffleMoves, false, mvSpeed);
	break;
    case 'R':			// Start redoing all a player's undone moves.
	nRMoves = moves.count() - (uint) (shuffleMoves + playerMoves);
	mvSpeed = viewMoves ? moveSpeed : 0;	// Avoid an ugly fast animation.
	startMoves (nRMoves, shuffleMoves + playerMoves, false, mvSpeed);
	playerMoves = playerMoves + nRMoves;
	break;
    case 'u':			// Start undoing a player's move.
	startMoves (1, shuffleMoves + playerMoves - 1, true, mvSpeed);
	playerMoves--;
	break;
    case 'U':			// Start undoing all the player's moves.
	mvSpeed = viewMoves ? moveSpeed : 0;	// Avoid an ugly fast animation.
	startMoves (playerMoves, shuffleMoves + playerMoves - 1, true, mvSpeed);
	playerMoves = 0;
	break;
    case 's':			// Start a solution sequence (undo shuffle).
	startMoves (shuffleMoves, shuffleMoves - 1, true, shSpeed);
	break;
    case 'w':			// WAIT (pause without changing the display).
	// 1 second if static, 2 seconds if tumbling (time to view all faces).
	pauseTicks = tumbling ? 100 : 50;
	break;
    }
    chooseMousePointer ();
}


bool Game::tooBusy ()
{
    if (tumbling || (movesToDo > 0) || (! displaySequence.isEmpty())) {
	KMessageBox::information (myParent,
		i18n("The cube has animated moves in progress "
		     "or the demo is running.\n\n"
		     "Please wait or click on the cube to stop the demo."),
		i18n("Sorry, too busy."));
	return (true);
    }
    else {
	return (false);
    }
}


void Game::startMoves (int nMoves, int index, bool pUndo, int speed)
{
    movesToDo = nMoves;
    moveIndex = index;
    undoing   = pUndo;
    moveFeedback  = None;

    Move * firstMove = moves.at (moveIndex);
    cube->setMoveInProgress (firstMove->axis, firstMove->slice);
    cube->setMoveAngle (0);
    startAnimatedMove (firstMove, speed);
}


void Game::startAnimatedMove (Move * move, int speed)
{
    // Update the Singmaster moves display, if not a shuffling move.
    if (moveIndex > (shuffleMoves - 1)) {
	// The "-" in the pattern must be just before the "]", otherwise it
	// defines a range of characters and does not get matched as a "-".

	QRegularExpression smPattern (QStringLiteral("[.C]*[FBLRUD]['2 +-]*"));
	// IDW testing - qCDebug(KUBRICK_LOG) << "Undoing" << undoing << singmasterString <<
			// IDW testing - smSelectionStart << smSelectionLength;
	if (undoing) {
	    int pos1 = 0;
	    int pos2 = smSelectionStart;
	    smSelectionStart = 0;
	    smSelectionLength = 0;
	    while (smSelectionStart < pos2) {
		QRegularExpressionMatch match = smPattern.match(singmasterString, pos1);
		pos1 = match.capturedStart();
		if ((pos1 >= pos2) || (pos1 < 0)) {
		    break;
		}
		smSelectionStart = pos1;
		smSelectionLength = match.capturedLength();
		pos1 = pos1 + smSelectionLength;
	    }
	}
	else {
	    int pos1 = smSelectionStart + smSelectionLength;
	    QRegularExpressionMatch match = smPattern.match(singmasterString, pos1);
	    int pos2 = match.capturedStart();
	    if (pos2 >= 0) {
		pos2 = pos2 + match.capturedLength();
		smSelectionStart = pos1;
		smSelectionLength = pos2 - pos1;
	    }
	}
	smShowSingmasterMoves();	// Re-display the Singmaster moves.
    }

    if (speed == 0) {
	return;			// No animation required.
    }

    // Set up the initial conditions, the advance() slot will do the rest.
    moveAngleMax  = move->degrees;		// 90 or 180 degrees.
    moveAngleStep = (move->direction == CLOCKWISE) ? +speed : -speed;
    if (undoing) {
	moveAngleStep = -moveAngleStep;
    }

    moveAngle     = 0;
}


void Game::startNextMove (int speed)
{
    // Save the effect of the move just completed in the Cube's state.
    Move * move  = moves.at (moveIndex);
    Rotation rot = move->direction;

    if (undoing && (rot != ONE_EIGHTY)) {
	rot = (rot == CLOCKWISE) ? ANTICLOCKWISE : CLOCKWISE;
    }
    cube->moveSlice (move->axis, move->slice, rot);

    // Re-initialise (clear) the animation angles.
    moveAngleMax  = 0;
    moveAngleStep = 0;
    moveAngle     = 0;

    if (movesToDo <= 0) {
	// NO MOVES LEFT.
	return;
    }

    if (undoing) {
	--moveIndex;
    }
    else {
	++moveIndex;
    }
    move = moves.at (moveIndex);

    cube->setMoveInProgress (move->axis, move->slice);
    cube->setMoveAngle (0);
    startAnimatedMove (move, speed);
}

#include "moc_game.cpp"
