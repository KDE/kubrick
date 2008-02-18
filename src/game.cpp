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

// Local includes
#include "game.h"


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
    currentButton = Qt::NoButton;	// No mouse button being pressed.
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

    // Set the scene parameters for 1, 2 or 3 cubes.  Each cube has ID, size,
    // co-ordinates of centre, turn, tilt, label location and label widget.

    setCubeView (1, TURNS, 2.0,  0.0,  0.0, -5.0, -45.0, +40.0, 0, 0, NoLabel);

    setCubeView (2, TURNS, 2.0, -1.3,  0.0, -5.0, -20.0, +40.0, 2, 0, FrontLbl);
    setCubeView (2, TURNS, 2.0, +1.3,  0.0, -5.0, 125.0, +30.0, 6, 0, BackLbl);

    setCubeView (3, TURNS, 2.0, -1.0,  0.0, -5.0, -30.0, +40.0, 0, 0, NoLabel);
    setCubeView (3, FIXED, 1.0, +1.7, +1.2, -5.0, -60.0, +40.0, 7, 0, FrontLbl);
    setCubeView (3, FIXED, 1.0, +1.7, -1.4, -5.0, 130.0, +40.0, 7, 4, BackLbl);

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
	// Calculate the RubikCube co-ordinate of the slice to rotate.
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
    blinking = FALSE;

    appendMove ();		// Add the move and start it off.
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

    cube = new RubikCube (myParent, cubeSize[X], cubeSize[Y], cubeSize[Z]);

    // Set default parameters for the first move (if using keyboard control).
    currentMoveAxis = Z;
    currentMoveSlice = cubeSize[Z] - 1;	// Front face (+Z).
    currentMoveDirection = CLOCKWISE;
    blinking = FALSE;			// No move being selected.
    foundFace1 = FALSE;			// No mouse-controlled move selected.
    blinkStartTick = 0;			// No blinking due to start.

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

	// At most one side should have size 1 (Rubik's mat?).
	if (count > 1) {
	    KMessageBox::information (myParent,
		    i18n("Only one of your dimensions can be one cubie wide."),
		    i18n("Rubik's Cube Options"));
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
    // Move the current position away from the origin, so that we can
    // draw objects near Z = 0 and still be able to see them OK.
    double aspect = (double) gameGLView->width () / gameGLView->height ();

    foreach (CubeView * v, cubeViews) {
	if (v->sceneID != currentSceneID)
	    continue;			// Skip unwanted scene IDs.

	gameGLView->pushGLMatrix ();
	gameGLView->moveGLView (v->position[X]*aspect,
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
		float x, float y, float z,
		float turn, float tilt, int labelX, int labelY, LabelID label)
{
    CubeView * v = new CubeView;

    v->sceneID      = sceneID;
    v->rotates      = rotates;
    v->size         = size;
    v->position [X] = x;
    v->position [Y] = y;
    v->position [Z] = z;
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
    blinking = TRUE;
}


void Game::appendMove ()
{
    Move * move          = new Move;

    // Add the player's move to the list.
    move->axis           = currentMoveAxis;
    move->slice          = currentMoveSlice;
    move->direction      = currentMoveDirection;
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

void Game::handleMouseEvent (int event, int button, int mX, int mY)
{
    double position [nAxes];
    bool   found = FALSE;

    // A mouse click (press and release) stops the demo.
    if (demoPhase) {
	if (event == -1) {
	    stopDemo ();
	    restoreState ();
	}
	return;
    }

    if (tooBusy())
	return;

    // If playing, find which picture of a cube the mouse is on.
    CubeView * v = cubeViews.at (findWhichCube (mX, mY));
    if (v == 0) {
	printf ("Game::handleMouseEvent: Could not find the nearest cube.\n");
	return;
    }

    // Get the mouse position relative to the centre of the cube we found.
    gameGLView->getGLPosition (mX, mY, v->matrix, position);

    // Find the sticker we are on (ie. the closest face-centre).
    int face [nAxes];
    found = cube->findSticker (position, v->cubieSize, face);
    currentButton = button;

    // After a button-press, save the cubie face-centre that was found.
    if (event == +1) {
	foundFace1 = found;			// Save the found-flag.
	if (foundFace1) {
	    cube->setBlinkingOff ();
	    blinking = FALSE;
	    LOOP (n, nAxes) {
		// Save the co-ordinates of the cubie face-centre.
		face1[n] = face[n];
		if (abs(face1 [n]) != cubeSize [n]) {
		    if (currentButton == Qt::LeftButton) {
			// Show the two slices that might move.
			cube->setBlinkingOn ((Axis) n, face1[n]);
		    }
		    else {
			cube->setBlinkingOn ((Axis) n, WHOLE_CUBE);
		    }
		}
	    }
	    blinkStartTick = nTick + 15;	// Set a delay before blinking
	    gameGLView->setBlinkIntensity (1.0); // and keep intensity high.
	}
    } // End - Button-press event

    // After a button-release, calculate the slice or whole-cube move required.
    if (event == -1) {
	int result = evaluateMove (found, face);
	foundFace1 = FALSE;
	cube->setBlinkingOff ();
	blinking = FALSE;
	if (result == 1) {
	    appendMove();		// Valid: add the move and start it.
	}
	else if (result < 0) {
	    // We found either one cubie or none.  Either the press or the
	    // release or both missed the cube and hit the background area.
	    KMessageBox::information (myParent,
		i18n("Sorry, you must start and finish with the mouse "
		     "pointer somewhere on a colored sticker, not in the "
		     "background area."),
		i18n("Move Error"), "move_error_1");
	}
	else if (result == 0) {
	    // If count == 0, the move was skewed between two slices.
	    KMessageBox::information (myParent,
		i18n("Sorry, to turn a slice of the cube you must hold "
		     "the left mouse button down and drag the pointer "
		     "from one colored sticker to another on the "
		     "same slice, or you can go around onto "
		     "another face of the cube.\n\nIf you try your move "
		     "again, but slowly, the cube will blink and show "
		     "you which slices can move.  When you have just one "
		     "slice blinking, that is the one that will move."),
		i18n("Move Error"), "move_error_2");
	}
	else if (result > 1) {
	    // If count > 1, the mouse always stayed on the same cubie.
	    KMessageBox::information (myParent,
		i18n("Sorry, to turn a slice of the cube you must drag "
		     "the mouse pointer from one colored sticker to "
		     "another, with the left button held down.  If you "
		     "stay on the same sticker, nothing happens."),
		i18n("Move Error"), "move_error_3");
	}
	currentButton = Qt::NoButton;

    } // End - Button-release event
}


void Game::showMouseMoveProgress ()
{
    double position [nAxes];
    QPoint p = gameGLView->getMousePosition ();

    // Find which picture of a cube the mouse is on.
    CubeView * v = cubeViews.at (findWhichCube (p.x(), p.y()));
    if (v == 0) {
	return;				// Not found: ignore problem.
    }
    else {
        gameGLView->getGLPosition (p.x(), p.y(), v->matrix, position);
    }

    // Find the sticker we are on (ie. the closest face-centre).
    int face [nAxes];
    bool found = cube->findSticker (position, v->cubieSize, face);

    if (evaluateMove (found, face) == 1) {
	// We have identified a single slice: make it blink.
	cube->setBlinkingOff ();
	cube->setBlinkingOn (currentMoveAxis, currentMoveSlice);
    }
    else if (foundFace1) {
	// We cannot identify a single slice: make two possible slices blink.
	cube->setBlinkingOff ();
	LOOP (n, nAxes) {
	    if (abs(face1 [n]) != cubeSize [n]) {
		if (currentButton == Qt::LeftButton) {
		    // Show the two slices that might move.
		    cube->setBlinkingOn ((Axis) n, face1[n]);
		}
		else {
		    cube->setBlinkingOn ((Axis) n, WHOLE_CUBE);
		}
	    }
	}
    }
}


int Game::evaluateMove (bool found, int face [])
{
    int count = 0;		// Result: 1 = OK, other values are error codes.

    if (found && foundFace1) {
	Axis axis = X;

	// Find the axis and slice for the mouse-guided move.
	LOOP (n, nAxes) {
	    // Check axes that are perpendicular to either cubie face.
	    if (abs(face[n]) == cubeSize [n]) {
		if (abs(face1[n]) == cubeSize [n]) {
		    // The mouse is on the same or opposite face of the
		    // cube: we need the row/column of BOTH mouse posns
		    // to identify which of two slices is to move, so
		    // the mouse needs to be exactly on that row/column.
		    break;
		}
		else {
		    // The mouse has gone around to a face of the cube
		    // that is at right angles to the first one: the 
		    // required slice is perpendicular to both faces.
		    // The mouse need not be on a cubie in that slice,
		    // so the user does not need to be so precise.

		    // Find the axis of the required slice.
		    axis = (Axis) ((n + 1) % nAxes);
		    if (abs(face1 [axis]) == cubeSize [axis]) {
			// The slice axis must be the next axis but one.
			axis = (Axis) ((n + 2) % nAxes);
		    }

		    // Lock on to the position of the required slice.
		    face [axis] = face1 [axis];
		    break;
		}
	    }
	}

	LOOP (n, nAxes) {
	    // Find face-centre coords that are the same on both cubies.
	    if ((face[n] == face1[n]) && (abs(face[n]) != cubeSize [n]))
	    {
		count++;
		axis = (Axis) n;
	    }
	}

	if (count == 1) {
	    // The move was between two separate cubies in a slice.  The
	    // coord that is the same in each face-centre identifies
	    // the slice and its axis is the required axis of rotation.
	    currentMoveAxis = axis;
	    if (currentButton == Qt::LeftButton) {
		currentMoveSlice = face[axis];
	    }
	    else {
		currentMoveSlice = WHOLE_CUBE;
	    }

	    // If the cross-product of face-centre vectors is positive
	    // along "axis", we rotate anticlockwise, else clockwise.

	    // First we compute the other two axes, in cyclical order.
	    Axis a1 = (Axis) ((currentMoveAxis + 1) % nAxes);
	    Axis a2 = (Axis) ((currentMoveAxis + 2) % nAxes);

	    if (((face1[a1] * face[a2]) - (face[a1] * face1[a2])) > 0) {
		currentMoveDirection = ANTICLOCKWISE;
	    }
	    else {
		currentMoveDirection = CLOCKWISE;
	    }
	}
	else {
	    // If count == 0, the move was skewed between two slices.
	    // If count > 1, the mouse always stayed on the same cubie.
	}
    }
    else {
	// We found either one cubie or none.  Either the press or the
	// release or both missed the cube and hit the background area.
	count = -1;
    }
    return (count);
}


int Game::findWhichCube (int mX, int mY)
{
    // For some reason this function cannot compile with return-type CubeView *.
    double aspect   = (double) gameGLView->width () / gameGLView->height ();
    double position [nAxes];
    double distance = 10000.0;		// Large value.
    double d        = 0.0;
    double dx       = 0.0;
    double dy       = 0.0;
    double dz       = 0.0;
    int    indexV   = -1;

    // Get the mouse position in OpenGL world co-ordinates.
    gameGLView->getAbsGLPosition (mX, mY, position);

    // Find which cube in the current scene is closest to the mouse position.
    CubeView * v;
    LOOP (n, cubeViews.size()) {
        v = cubeViews.at (n);
	if (v->sceneID != currentSceneID)
	    continue;			// Skip unwanted scene IDs.

	dx = position[X] - v->position[X]*aspect;
	dy = position[Y] - v->position[Y];
	dz = position[Z] - v->position[Z];
	d  = sqrt (dx*dx + dy*dy + dz*dz);	// Pythagoras.
	if (d < distance) {
	    distance = d;
	    indexV   = n;
	}
    }
    return (indexV);
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

    // If blinking, to show player's next move, change intensity every 100 msec.
    if (nTick % 5 == 0) {
	if ((! blinking) && foundFace1 && (nTick > blinkStartTick)) {
	    blinking = TRUE;			// Delayed blink on mouse press.
	}
	if (blinking) {
	    if (foundFace1) {
		showMouseMoveProgress ();	// Mouse-controlled move.
	    }
	    // Blink slice(s) when selecting a move using mouse or keyboard.
	    gameGLView->setBlinkIntensity (((float) ((nTick/5)%3) * 0.1) + 0.5);
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
		startNextMove (TRUE);
	    }
	    else {
		// Do all the moves at once, without any animation.
		movesToDo--;
		startNextMove (FALSE);
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

    // Initiate the required sequence of moves (or single move).
    switch (c) {
    case 'd':			// Start a random demo sequence ("whwswd").
	randomDemo ();
	break;
    case 'h':			// Start a shuffling sequence of moves.
	startMoves (shuffleMoves, 0, FALSE, viewShuffle);
	break;
    case 'm':			// Start doing or
    case 'r':			// redoing a player's move.
	playerMoves++;
	startMoves (1, shuffleMoves + playerMoves - 1, FALSE, viewMoves);
	break;
    case 'R':			// Start redoing all a player's undone moves.
	nRMoves = moves.count() - (uint) (shuffleMoves + playerMoves);
	startMoves (nRMoves, shuffleMoves + playerMoves, FALSE, viewMoves);
	playerMoves = playerMoves + nRMoves;
	break;
    case 'u':			// Start undoing a player's move.
	startMoves (1, shuffleMoves + playerMoves - 1, TRUE, viewMoves);
	playerMoves--;
	break;
    case 'U':			// Start undoing all the player's moves.
	startMoves (playerMoves, shuffleMoves + playerMoves - 1,
			TRUE, viewMoves);
	playerMoves = 0;
	break;
    case 's':			// Start a solution sequence (undo shuffle).
	startMoves (shuffleMoves, shuffleMoves - 1, TRUE, viewShuffle);
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


void Game::startMoves (int nMoves, int index, bool pUndo, bool animation)
{
    movesToDo = nMoves;
    moveIndex = index;
    undoing   = pUndo;
    blinking  = FALSE;
    Move * firstMove = moves.at (moveIndex);
    cube->setNextMove (firstMove->axis, firstMove->slice);
    startAnimatedMove (firstMove, animation);
}


void Game::startAnimatedMove (Move * move, bool animation)
{
    if (! animation) {
	return;
    }

    // Set up the initial conditions, the advance() slot will do the rest.
    moveAngleMax  = move->degrees;		// 90 or 180 degrees.
    moveAngleStep = (move->direction == CLOCKWISE) ? +moveSpeed : -moveSpeed;
    if (undoing) {
	moveAngleStep = -moveAngleStep;
    }
    moveAngle     = 0;
}


void Game::startNextMove (bool animation)
{
    // Save the effect of the move just completed in the RubikCube's state.
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
    startAnimatedMove (move, animation);
}

// Create a Rubik's cube

RubikCube::RubikCube (QWidget* parent, int xlen, int ylen, int zlen)
	: QObject(parent)
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
    nextMoveAxis   = Z;		// Front face (+Z).
    nextMoveSlice  = sizes[Z] - 1;
}

RubikCube::~RubikCube ()
{
    while (! cubies.isEmpty()) {
        delete cubies.takeFirst();
    }
}

void RubikCube::moveSlice (Axis axis, int location, Rotation direction)
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

void RubikCube::addStickers ()
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


void RubikCube::drawCube (GameGLView * gameGLView, float cubieSize, int angle)
{
    // For each cubie in the cube ...
    foreach (Cubie * cubie, cubies) {

	if (cubie->hasNoStickers()) {
	    // This cubie is deep inside the cube: save time by not drawing it.
	    continue;
	}

	// Draw the cubie and its stickers.
	cubie->drawCubie (gameGLView, cubieSize,
			  nextMoveAxis, nextMoveSlice, angle);
    }
}


bool RubikCube::findSticker (double position [], float myCubieSize,
				int faceCentre [])
{
    bool             result       = FALSE;
    double           location [nAxes];
    double           distance     = sqrt ((double) 2.0);
    double	     d;

    LOOP (i, nAxes) {
	location [i] = (position [i] / myCubieSize) * 2.0;
    }

    foreach (Cubie * cubie, cubies) {
	d = cubie->findCloserSticker (distance, location, faceCentre);
	if (d < distance) {
	    distance = d;
	    result = TRUE;
	}
    }

    return (result);
}


void RubikCube::setNextMove (Axis axis, int location)
{
    setBlinkingOff ();
    nextMoveAxis   = axis;
    nextMoveSlice  = location;
}


void RubikCube::setBlinkingOn (Axis axis, int location)
{
    foreach (Cubie * cubie, cubies) {
	cubie->setBlinkingOn (axis, location, sizes[axis]);
    }
}


void RubikCube::setBlinkingOff ()
{
    foreach (Cubie * cubie, cubies) {
	cubie->setBlinkingOff ();
    }
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
    s->blinking = FALSE;
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
			int nextMoveAxis, int nextMoveSlice, int angle)
{
    float centre     [nAxes];

    // Calculate the centre of the cubie in OpenGL co-ordinates.
    LOOP (i, nAxes) {
	centre [i] = ((float) currentCentre [i]) * cubieSize / 2.0;
    }

    // If this cubie is in a moving slice, set its animation angle.
    int   myAngle = 0;
    if ((angle != 0) && ((nextMoveSlice == WHOLE_CUBE) ||
	(currentCentre [nextMoveAxis] == nextMoveSlice))) {
	myAngle = angle;
    }

    // Draw this cubie in color zero (grey plastic color).
    gameGLView->drawACubie (cubieSize, centre, nextMoveAxis, myAngle);

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
	    sticker->blinking = TRUE;
	}
    }
}


void Cubie::setBlinkingOff ()
{
    foreach (Sticker * sticker, stickers) {
	sticker->blinking = FALSE;
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
    bool moved = FALSE;

    // Check if the cubie's centre is in a new position.
    LOOP (i, nAxes) {
	if (currentCentre  [i] != originalCentre [i])
	    moved = TRUE;
    }

    // Check if the cubie is back where it was but has been given a twist.
    if (! moved) {
	foreach (Sticker * s, stickers) {
	    LOOP (i, nAxes) {
		if (s->currentFaceCentre [i] != s->originalFaceCentre [i])
		    moved = TRUE;
	    }
	}
    }

    // If anything has changed, print the cubie.
    if (moved) {
	printAll ();
    }
}
