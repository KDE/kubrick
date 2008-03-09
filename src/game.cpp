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

#include <KDebug>

// Local includes
#include "game.h"
#include "movetracker.h"


// Create the main game/document object
Game::Game (Kubrick * parent)
	: QObject (parent),
          moveIndex (-1)
{
    myParent = parent;
    cube = 0;
    gameGLView = 0;			// OpenGL view is not yet created.
    mainWindow = 0;			// MW exists, but the GUI is not set up.

    random.setSeed (0);			// Zero gets us an arbitrary seed.

    setDefaults ();			// Set all options to default values.
    restoreState ();			// Restore the last cube and its state.
    demoPhase = FALSE;			// No demo yet.

    moveTracker = new MoveTracker (myParent);

    connect (moveTracker, SIGNAL(newMove (Move *)),
			this, SLOT(appendMove(Move *)));

    blinkStartTime = 300;
}


Game::~Game ()
{
    while (! cubeViews.isEmpty()) {
        delete cubeViews.takeFirst();
    }
    while (! moves.isEmpty()) {
        delete moves.takeFirst();
    }
}


void Game::initGame (GameGLView * glv, Kubrick * mw)
{
    // OK, now we have an OpenGL view and all the GUI widgets in KXmlGuiWindow.
    gameGLView = glv;			// Save OpenGL view, used when drawing.
    mainWindow = mw;			// Save main window, shows status, etc.

    mainWindow->setToggle ("toggle_tumbling", tumbling);
    mainWindow->setToggle ("watch_shuffling", (bool) option [optViewShuffle]);
    mainWindow->setToggle ("watch_moves",     (bool) option [optViewMoves]);
    gameGLView->setBevelAmount (option [optBevel]);
    gameGLView->setCursor (Qt::CrossCursor);

    // Create the view-label texts for cube pictures.
    frontVL = gameGLView->addLabel (i18n("Front View"));
    backVL  = gameGLView->addLabel (i18n("Back View"));

    // Don't show them at startup time.
    frontVL-> hide ();
    backVL->  hide ();

    demoL  =  gameGLView->addLabel
			(i18n("DEMO - Click anywhere to stop"));
    demoL->   move (10, 10);
    demoL->   hide ();			// Show it whenever the demo starts.

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

    // This will clear animation variables and do no more.
    startAnimation ("", option [optSceneID], FALSE, FALSE);
    while (! moves.isEmpty()) {
        delete moves.takeFirst();
    }
    shuffleMoves = 0;
    playerMoves = 0;

    startDemo ();			// Start the demo.
    randomDemo ();

    // Implement game ticks [SLOT(advance()) does most of the work in Kubrick].
    QTimer* timer = new QTimer (this);
    nTick             = 0;
    connect (timer, SIGNAL(timeout()), this, SLOT(advance()));

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
		     "menu item Game->Choose Puzzle Type->Make Your Own..."),
		i18n("New Puzzle"));
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
    QString loadFilename = KFileDialog::getOpenFileName (KUrl(),
			    "*.kbk", myParent, i18n("Load Puzzle"));
    if (loadFilename.isNull()) {
	return;
    }

    KConfig config (loadFilename, KConfig::SimpleConfig);

    // printf ("Game::load (%s)\n", loadFilename.toLatin1());
    if (! config.hasGroup ("KubrickGame")) {
	printf ("File '%s' is not a valid Kubrick game-file.\n",
				       loadFilename.toLatin1().data());
	return;
    }

    loadPuzzle (config);
}


void Game::save ()
{
    doSave (FALSE);			// Use previous file name, if available.
}


void Game::saveAs ()
{
    doSave (TRUE);			// Use the file dialog to get a name.
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
    mainWindow->setToggle ("watch_shuffling", (bool) option [optViewShuffle]);
    mainWindow->setToggle ("watch_moves",     (bool) option [optViewMoves]);

    // Build the new cube and shuffle it.
    newCube (option [optXDim], option [optYDim], option [optZDim],
			       option [optShuffleMoves]);
}


void Game::newCubeDialog ()
{
    if (demoPhase) {
	toggleDemo();
    }
    if (doOptionsDialog (TRUE) == QDialog::Accepted) {
	newCube (option [optXDim], option [optYDim], option [optZDim],
				   option [optShuffleMoves]);
    }
}


void Game::undoMove ()
{
    startUndo ("u", i18n("Undo"));
}


void Game::redoMove ()
{
    startRedo ("r", i18n("Redo"));
}


void Game::solveCube ()
{
    if (tooBusy())
	return;
    if (shuffleMoves <= 0) {
	KMessageBox::information (myParent,
		i18n("This cube has not been shuffled, so there is "
		     "nothing to solve."),
		i18n("Solve the Cube"));
	return;
    }

    if (playerMoves > 0) {
	// Undo player moves, wait, solve (undo shuffle), wait, redo shuffle.
	startAnimation ("Uwswh", option [optSceneID], TRUE, FALSE);
    }
    else {
	// No player moves: solve (undo shuffle), wait, redo shuffle.
	startAnimation ("swh", option [optSceneID], TRUE, FALSE);
    }
}


void Game::toggleTumbling ()
{
    // Start or stop the tumbling motion of the cube(s) displayed.
    tumbling = ! tumbling;
    chooseMousePointer ();
}


void Game::setZeroTumbling ()
{
    tumblingTicks = 0;
}


void Game::undoAll ()
{
    startUndo ("U", i18n("Restart Puzzle (Undo All)"));
}


void Game::redoAll ()
{
    startRedo ("R", i18n("Redo All"));
}


void Game::cycleSceneUp ()
{
    // Add a cube to the view or cycle forward to a 1-cube view.
    currentSceneID = (currentSceneID < (nSceneIDs - 1)) ? (currentSceneID + 1)
						  : OneCube;
    option [optSceneID] = currentSceneID;
    setSceneLabels ();
}


void Game::cycleSceneDown ()
{
    // Remove a cube from the view or cycle back to a 3-cube view.
    currentSceneID = (currentSceneID > 1) ? (currentSceneID - 1) : ThreeCubes;
    option [optSceneID] = currentSceneID;
    setSceneLabels ();
}


void Game::toggleDemo ()
{
    if (demoPhase) {
	stopDemo ();
	restoreState ();
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
    QString demoFile = KStandardDirs::locate ("appdata", file);
    KConfig config (demoFile, KConfig::SimpleConfig);
    if (config.hasGroup ("KubrickGame")) {
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
		i18n("File Not Found"));
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
    (void) doOptionsDialog (FALSE);
}


void Game::setMoveAxis (int i)
{
    if (tooBusy())
	return;
    // Triggered by key x, y or z.
    currentMoveAxis = (Axis) i;
    startBlinking ();
}


void Game::setMoveSlice (int slice)
{
    if (tooBusy())
	return;
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
    // Triggered by LeftArrow or RightArrow key.
    currentMoveDirection = (Rotation) direction;
    cube->setBlinkingOff ();
    moveFeedback = None;

    Move * move          = new Move;

    move->axis           = currentMoveAxis;
    move->slice          = currentMoveSlice;
    move->direction      = currentMoveDirection;

    appendMove (move);			// Add the move and start it off.
}


void Game::setSceneLabels ()
{
    int		x, y;
    int		w = gameGLView->width();
    int		h = gameGLView->height();
    QLabel *	labelObj = 0;

    frontVL->hide();
    backVL->hide();

    foreach (CubeView * v, cubeViews) {
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
	y = (v->labelY * h)/8 + 10;
	labelObj->move (x, y);
	labelObj->show ();
    }
}


void Game::saveState ()
{
    if (demoPhase) {
	return;				// Don't save if quitting during a demo.
    }
    QString sFile = KStandardDirs::locateLocal ("appdata", "kubrick.save");
    KConfig config (sFile, KConfig::SimpleConfig);
    savePuzzle (config);
}


void Game::restoreState ()
{
    QString rFile = KStandardDirs::locateLocal ("appdata", "kubrick.save");
    KConfig config (rFile, KConfig::SimpleConfig);
    if (config.hasGroup ("KubrickGame")) {
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
    if (cube != 0) {
	delete cube;			// Delete the previous cube (if any).
    }
    cubeSize [X] = xDim;
    cubeSize [Y] = yDim;
    cubeSize [Z] = zDim;
    nMax = (xDim > yDim) ? xDim: yDim;
    nMax = (zDim > nMax) ? zDim: nMax;
    shuffleMoves = shMoves;

    cube = new Cube (this, cubeSize[X], cubeSize[Y], cubeSize[Z]);

    // Set default parameters for the first move (if using keyboard control).
    currentMoveAxis = Z;
    currentMoveSlice = cubeSize[Z] - 1;	// Front face (+Z).
    currentMoveDirection = CLOCKWISE;

    moveFeedback = None;		// No move being selected.

    while (! moves.isEmpty()) {
        delete moves.takeFirst();
    }
    playerMoves = 0;

    // Shuffle the cube.
    QString dSeq = "";			// No moves to do, if no shuffling.
    if (shuffleMoves > 0) {
	shuffleCube ();			// Calculate the shuffling moves.
	dSeq = "h";			// Ask to do the shuffling moves.
    }
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
		    i18n("Cube Options"));
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
	mainWindow->setToggle ("watch_shuffling", (bool)option[optViewShuffle]);
	mainWindow->setToggle ("watch_moves",     (bool)option[optViewMoves]);
    }

    delete d;

    return (result);
}


void Game::drawScene ()
{
    // The size of the cubes on-screen is determined by the height of the
    // window and the OpenGL perspective projection, in which the window-height
    // always has a set angle of view.  The lateral spacing of the cubes (X
    // co-ordinates of centres) is scaled by the aspect-ratio, to give us an
    // even spacing however wide the window is.

    double aspect = (double) gameGLView->width () / gameGLView->height ();
    float fieldHeight = -cubeCentreZ * 2.0 * tan (3.14159 * viewAngle/360.0);
    float fieldWidth  = aspect * fieldHeight;

    foreach (CubeView * v, cubeViews) {
	if (v->sceneID != currentSceneID)
	    continue;			// Skip unwanted scene IDs.

	v->position [X] = v->relX * fieldWidth;
	v->position [Y] = v->relY * fieldHeight;
	v->position [Z] = cubeCentreZ;

	gameGLView->pushGLMatrix ();
	gameGLView->moveGLView (v->position[X], // IDW *** *aspect,
				v->position[Y],
				v->position[Z]);
	v->cubieSize = v->size / nMax;

	// Tumble the cube if this cube can rotate.
	if (v->rotates) {
	    tumble ();
	}

	// Turn and tilt, to make 3 faces visible.
	gameGLView->rotateGLView (v->turn, 0.0, 1.0, 0.0);
	gameGLView->rotateGLView (v->tilt, 1.0, 0.0, -1.0);

	// Save the matrix for this view of the cube.
	glGetDoublev (GL_MODELVIEW_MATRIX, v->matrix);

	cube->drawCube (gameGLView, v->cubieSize, moveAngle);

	gameGLView->popGLMatrix ();
    }
}


void Game::setCubeView (int sceneID, bool rotates, float size,
		float relX, float relY,
		float turn, float tilt, int labelX, int labelY, LabelID label)
{
    CubeView * v = new CubeView;

    double aspect = (double) gameGLView->width () / gameGLView->height ();
    float fieldHeight = -cubeCentreZ * 2.0 * tan (3.14159 * viewAngle/360.0);
    float fieldWidth  = aspect * fieldHeight;
    printf ("H = %8.2f, W = %8.2f, aspect = %8.2f\n",
			fieldHeight, fieldWidth, aspect);

    v->sceneID      = sceneID;
    v->rotates      = rotates;
    v->size         = size;
    v->relX         = relX;
    v->relY         = relY;
    v->position [X] = v->relX * fieldWidth;
    v->position [Y] = v->relY * fieldHeight;
    v->position [Z] = cubeCentreZ;
    printf ("X = %8.2f, Y = %8.2f, Z = %8.2f\n",
		v->position [X], v->position [Y], v->position [Z]);
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
    mainWindow->setToggle (KStandardGameAction::name (KStandardGameAction::Demo), TRUE);
    // Disable the Save actions.
    mainWindow->setAvail (KStandardGameAction::name (KStandardGameAction::Save),   FALSE);
    mainWindow->setAvail (KStandardGameAction::name (KStandardGameAction::SaveAs), FALSE);
    // Disable the Preferences action.
    mainWindow->setAvail (KStandardAction::name (KStandardAction::Preferences), FALSE);

    demoPhase = TRUE;
    demoL->show ();			// Show the "click to stop" message.
}


void Game::randomDemo ()
{
    double pickShape = random.getDouble ();

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
    tumbling          = TRUE;
    mainWindow->setToggle ("toggle_tumbling", tumbling);
    moveSpeed         = 5;

    frontVL-> hide ();			// Hide the view-labels.
    backVL->  hide ();

    // Create the demo cube.
    newCube (cubeSize [X], cubeSize [Y], cubeSize [Z], shuffleMoves);

    // Shuffle, solve, start next demo ... 1 cube in scene, all moves animated.
    startAnimation ("whwswd", OneCube, TRUE, TRUE);
}


void Game::stopDemo ()
{
    // Set the demo's toggle-button OFF.
    mainWindow->setToggle (KStandardGameAction::name (KStandardGameAction::Demo), FALSE);
    // Enable the Save actions.
    mainWindow->setAvail (KStandardGameAction::name (KStandardGameAction::Save),   TRUE);
    mainWindow->setAvail (KStandardGameAction::name (KStandardGameAction::SaveAs), TRUE);
    // Enable the Preferences action.
    mainWindow->setAvail (KStandardAction::name (KStandardAction::Preferences), TRUE);

    tumbling = FALSE;
    mainWindow->setToggle ("toggle_tumbling", tumbling);

    demoL->hide ();			// Hide the DEMO text.
    demoPhase = FALSE;
}


void Game::setDefaults()
{
    // Set default values for the options.

    option [optXDim] = 3;		// Cube size 3x3x3.
    option [optYDim] = 3;
    option [optZDim] = 3;

    option [optShuffleMoves] = 4;	// Four shuffling moves.
    option [optViewShuffle] = (int) TRUE; // Animate the shuffling moves.
    option [optViewMoves] = (int) FALSE;  // Don't animate the player's moves.
    option [optBevel] = 12;		// 12% bevel on edges of cubies.
    option [optMoveSpeed] = 5;		// Speed of moves (5 deg/tick) [1..10].

    option [optSceneID] = 2;		// Scene: 2 cubes, front and back views.
    option [optTumbling] = (int) FALSE;	// No tumbling.
    option [optTumblingTicks] = 0;	// "Home" orientation.
    option [optMouseBlink] = (int) TRUE;  // Blink during mouse-controlled move.
}


void Game::startBlinking ()
{
    cube->setBlinkingOff ();
    cube->setBlinkingOn (currentMoveAxis, currentMoveSlice);
    moveFeedback = Keyboard;
    time.start();
    kDebug() << "Keyboard time start.";
}


void Game::appendMove (Move * move)
{
    move->degrees        = 90;

    // If single slice and not square, rotate 180 degrees rather than 90.
    if ((currentMoveSlice != WHOLE_CUBE) &&
	(cubeSize [(move->axis+1)%nAxes] != cubeSize [(move->axis+2)%nAxes])) {
	move->degrees = 180;
    }

    while (moves.count() > (shuffleMoves + playerMoves)) {
	moves.removeLast();	// Remove undone moves (if any).
    }

    moves.append (move);

    // Start the move off.
    startAnimation ("m", option [optSceneID], option [optViewShuffle],
					      option [optViewMoves]);
}


void Game::doSave (bool getFilename)
{
    if (demoPhase || tooBusy())
	return;
    if (saveFilename.isEmpty() || getFilename) {
	QString newFilename = KFileDialog::getSaveFileName (KUrl(),
				"*.kbk", myParent, i18n("Save Puzzle"));
	if (newFilename.isNull()) {
	    return;
	}
	saveFilename = newFilename;
    }
    // printf ("File name is %s\n", saveFilename.toLatin1());

    KConfig config (saveFilename, KConfig::SimpleConfig);
    savePuzzle (config);
}


void Game::savePuzzle (KConfig & config)
{
    // Clear any previously saved info (in case there are fewer moves now).
    config.deleteGroup ("KubrickGame");
    KConfigGroup configGroup = config.group("KubrickGame");

    QStringList list;
    QString     value;

    // Save the option settings.
    LOOP (i, nOptions) {
	value.sprintf ("%d", option [i]);
	list.append (value);
    }
    configGroup.writeEntry ("a) Options", list);

    // Save the display sequence.
    // printf ("Save Display: [%s], Counts: %d, %d, %d\n",
	// displaySequence.toLatin1(), shuffleMoves, playerMoves, moves.count());
    configGroup.writeEntry ("c) DisplaySequence", displaySequence);
    QString dsTemp = configGroup.readEntry ("c) DisplaySequence", "");

    // Save the current move counts.
    list.clear ();
    value.sprintf ("%d", shuffleMoves);
    list.append (value);
    value.sprintf ("%d", playerMoves);
    list.append (value);
    value.sprintf ("%d", moves.count());
    list.append (value);
    configGroup.writeEntry ("f) MoveCounts", list);

    // Save the list of moves, using names "m) 001", "m) 002", etc.
    int n = 0;
    list.clear ();
    foreach (Move * m, moves) {
	value.sprintf ("%d", (int) m->axis);
	list.append (value);
	value.sprintf ("%d", m->slice);
	list.append (value);
	value.sprintf ("%d", (int) m->direction);
	list.append (value);
	value.sprintf ("%d", m->degrees);
	list.append (value);

	n++;
	value.sprintf ("m) %03d", n);
	configGroup.writeEntry (value, list);
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

    KConfigGroup configGroup = config.group ("KubrickGame");

    list = configGroup.readEntry ("a) Options", notFound);
    int nOpt = 0;
    for (it = list.begin(); it != list.end(); ++it) {
	optionTemp [nOpt] = (*it).toInt();
	// printf ("Option %d = %d\n", nOpt, optionTemp [nOpt]);
	nOpt++;
    }

    dsTemp = configGroup.readEntry ("c) DisplaySequence", "");
    // printf ("DisplaySequence = %s\n", dsTemp.toLatin1());

    list = configGroup.readEntry ("f) MoveCounts", notFound);
    int n = 0;
    for (it = list.begin(); it != list.end(); ++it) {
	moveCounts [n] = (*it).toInt();
	// printf ("Move count %d = %d\n", n, moveCounts [n]);
	n++;
    }

    QString key;
    for (n = 0; n < moveCounts [2]; n++) {
	key.sprintf ("m) %03d", n + 1);
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
	// printf ("Move %03d: %d %d %d %d\n", n + 1,
		// (int) moveTemp->axis, moveTemp->slice,
		// (int) moveTemp->direction, moveTemp->degrees);
    }

    LOOP (n, nOpt) {
	option [n] = optionTemp [n];
    }

    // Build the new cube but don't shuffle it (yet).
    newCube (option [optXDim], option [optYDim], option [optZDim], 0);

    moveSpeed		= option [optMoveSpeed];
    if (gameGLView != 0) {
	gameGLView->setBevelAmount (option [optBevel]);
    }
    tumbling		= option [optTumbling];
    tumblingTicks	= option [optTumblingTicks];
    if (mainWindow != 0) {
	mainWindow->setToggle ("toggle_tumbling", tumbling);
	mainWindow->setToggle ("watch_shuffling", (bool)option[optViewShuffle]);
	mainWindow->setToggle ("watch_moves",     (bool)option[optViewMoves]);
    }

    while (! moves.isEmpty()) {
        delete moves.takeFirst();
    }
    moves               = movesTemp;
    shuffleMoves	= moveCounts [0];
    playerMoves		= moveCounts [1];

    // If there are no saved moves and the cube should be shuffled, do it now.
    if ((moveCounts [2] == 0) && (option [optShuffleMoves] > 0)) {
	shuffleMoves = option [optShuffleMoves];
	shuffleCube ();
    }

    // Set the required display sequence, reconstructing it if necessary.
    QString dSeq = dsTemp;
    // printf ("Setting display sequence ... [%s] shuffle %d, play %d\n",
			// dSeq.toLatin1(), shuffleMoves, playerMoves);
    if (dSeq.isEmpty ()) {
	if (shuffleMoves > 0) {
	    dSeq = "h";
	}
	if (playerMoves > 0) {
	    // Redo all the player moves, using repeated "m" (not "R", Redo
	    // All), in case there are any undone moves on the end of the list.
	    QString repeats;
	    dSeq = dSeq + repeats.fill ('m', playerMoves);
	    playerMoves = 0;		// Start from the first player move.
	}
    }
    // printf ("Display sequence: %s\n", dSeq.toLatin1());

    // If dSeq is empty, we will clear previous animations and do no more,
    // otherwise we will reconstruct the saved position by re-doing all moves.

    if (demoPhase) {
	// Always animate demo sequences (pretty patterns or solving moves).
	startAnimation (dSeq, option [optSceneID], option [optViewShuffle],
						   option [optViewMoves]);
    }
    else {
	// Never re-animate restoreState() or other saved user files.
	startAnimation (dSeq, option [optSceneID], FALSE, FALSE);
    }

    if (gameGLView != 0) {
	advance ();			// Do all moves or just one anim step.
	chooseMousePointer ();		// Choose busy or idle mouse pointer.
	setSceneLabels ();		// Position the labels.
    }
}


void Game::startUndo (QString code, QString header)
{
    if (tooBusy())
	return;
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


void Game::startRedo (QString code, QString header)
{
    if (tooBusy())
	return;
    if (moves.count() > (shuffleMoves + playerMoves)) {
	// Start the redo off.
	startAnimation (code, option [optSceneID], option [optViewShuffle],
						   option [optViewMoves]);
    }
    else {
	KMessageBox::information (myParent,
		i18n("There are no moves to redo.\n\nThat could be because "
		     "you have not made any or you have redone them all or "
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
	kDebug() << "Mouse time start.";
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
    return (lo + (int) random.getLong (hi - lo + 1));
}


void Game::shuffleCube ()
{
    Move * move;
    Move * prev;
    int    sliceNo;

    // Empty the list of moves and delete all move objects.
    while (! moves.isEmpty()) {
        delete moves.takeFirst();
    }

    // printf ("\n%dx%dx%d cube, %d shuffle moves\n",
			// cubeSize[0], cubeSize[0], cubeSize[0], shuffleMoves);

    LOOP (n, shuffleMoves) {
	move = new Move;

	while (TRUE) {
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

	    // printf ("Move %ld, %2d: %3d %3d %3d %3d\n", (long) move,
	        // n, move->axis, move->slice, move->direction, move->degrees);

	    // Always accept the first move (n = 0).
	    if (n > 0) {
                prev = moves.at (n-1);		// Look at the last move.
		if ((move->axis  == prev->axis) &&
		    (move->slice == prev->slice) &&
		    ((move->degrees   == 180) ||
		     (move->direction != prev->direction))) {
		    // Reject a move that reverses the previous move.
		    // printf ("Move reverses previous move.\n");
		    continue;
		}
	    }
	    if (n > 1) {
		if (move->axis == prev->axis) {	// Axis same as in last move?
                    prev = moves.at (n-2);	// Look at the last but one.
		    if (move->axis == prev->axis) {
			// Reject a third successive move around the same axis.
			// printf ("Three successive moves around one axis.\n");
			continue;
		    }
		}
	    }
	    break;
	}

	moves.append (move);
	// printf ("Appended %ld %2d: %3d %3d %3d %3d\n", (long) move,
	    // n, move->axis, move->slice, move->direction, move->degrees);
    }
}

void Game::startAnimation (QString dSeq, int sID, bool vShuffle, bool vMoves)
{
    // Set the scene ID, animation display sequence and whether to animate the
    // shuffle and/or player moves or do them instantly (within one tick).

    displaySequence = dSeq;
    currentSceneID  = sID;
    viewShuffle     = vShuffle;
    viewMoves       = vMoves;
    // printf ("startAnimation (): [%s] scene %d viewShuffle %d viewMoves %d\n",
	// displaySequence.toLatin1(), currentSceneID, viewShuffle, viewMoves);

    // Initialise Game::advance().
    movesToDo       = 0;
    pauseTicks      = 0;
    undoing         = FALSE;

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
	// IDW kDebug() << "Time:" << t << "Blink start time:" << blinkStartTime;
	if (t >= blinkStartTime) {
	    // Blink slice(s) when selecting a move, whether using mouse or K/B.
	    gameGLView->setBlinkIntensity (((float) ((nTick/5)%3) * 0.1) + 0.5);

	    if (moveFeedback == Mouse) {
		// Position in window uses OpenGL convention (y = 0 at bottom).
		QPoint p = gameGLView->getMousePosition();
		moveTracker->mouseInput (currentSceneID, cubeViews, cube,
			Tracking, 0, p.x(), p.y());
	    }
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
		    break;		// Let the animation continue.
		}
		// If that animated move is completed, start the next one.
		movesToDo--;
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

    QTimer::singleShot(0, gameGLView, SLOT (updateGL()));
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
    char c = displaySequence.at(0).toAscii();
    displaySequence.remove (0, 1);
    int nRMoves = 0;

    // Set the animation speed: 0 = no animation, 15 = fastest.  Note that if
    // the "Watch Your Own Moves" option is off, animation is set to fastest. 
    int shSpeed = viewShuffle ? moveSpeed : 0;
    int mvSpeed = viewMoves   ? moveSpeed : 23;

    // Initiate the required sequence of moves (or single move).
    switch (c) {
    case 'd':			// Start a random demo sequence ("whwswd").
	randomDemo ();
	break;
    case 'h':			// Start a shuffling sequence of moves.
	startMoves (shuffleMoves, 0, FALSE, shSpeed);
	break;
    case 'm':			// Start doing or
    case 'r':			// redoing a player's move.
	playerMoves++;
	startMoves (1, shuffleMoves + playerMoves - 1, FALSE, mvSpeed);
	break;
    case 'R':			// Start redoing all a player's undone moves.
	nRMoves = moves.count() - (uint) (shuffleMoves + playerMoves);
	startMoves (nRMoves, shuffleMoves + playerMoves, FALSE, mvSpeed);
	playerMoves = playerMoves + nRMoves;
	break;
    case 'u':			// Start undoing a player's move.
	startMoves (1, shuffleMoves + playerMoves - 1, TRUE, mvSpeed);
	playerMoves--;
	break;
    case 'U':			// Start undoing all the player's moves.
	startMoves (playerMoves, shuffleMoves + playerMoves - 1, TRUE, mvSpeed);
	playerMoves = 0;
	break;
    case 's':			// Start a solution sequence (undo shuffle).
	startMoves (shuffleMoves, shuffleMoves - 1, TRUE, shSpeed);
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
		i18n("Sorry, the cube has animated moves in progress, or it is "
		     "tumbling, or the demo is running.\n\n"
		     "Please wait, click on the cube to stop the demo or "
		     "press the tumbling key (default key T)."),
		i18n("Sorry, too busy."), "too_busy");
	return (TRUE);
    }
    else {
	return (FALSE);
    }
}


void Game::startMoves (int nMoves, int index, bool pUndo, int speed)
{
    movesToDo = nMoves;
    moveIndex = index;
    undoing   = pUndo;
    moveFeedback  = None;
    Move * firstMove = moves.at (moveIndex);
    cube->setNextMove (firstMove->axis, firstMove->slice);
    startAnimatedMove (firstMove, speed);
}


void Game::startAnimatedMove (Move * move, int speed)
{
    if (speed == 0) {
	return;
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

    cube->setNextMove (move->axis, move->slice);
    startAnimatedMove (move, speed);
}
