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

// Qt includes.
#include <QDir>
#include <QString>
#include <QAction>
#include <QSignalMapper>

// KDE includes.
#include <KLocale>
#include <kstandardgameaction.h>
#include <KToggleAction>
#include <KActionCollection>
#include <KStatusBar>
#include <KStandardDirs>
#include <KMessageBox>
#include <KDebug>
#include <KShortcutsDialog>

// Local includes.
#include "kubrick.h"
#include "game.h"
#include "gameglview.h"

// Shorthand for reference to actions.
#define ACTION(x)   (actionCollection()->action(x))

Kubrick::Kubrick ()
{
    // Window title.
    // setCaption("Rubik's Cube");	// DELETED - This is a *trademark*.

    game     = new Game     (this);

    gameView = new GameGLView (game, this);

    // Set the view as the central widget
    setCentralWidget(gameView);

    // Set up the menus, keystrokes, etc.
    initGUI();

    // Enable the help menu.
    setHelpMenuEnabled (true);

    // Load the GUI from the kubrickui.rc file.
    setupGUI ();

    // Set up a status bar.
    statusBar()->show ();
    statusBar()->insertItem (i18n("Welcome to Kubrick"), 1001, 1);

    // IDW toolBar("mainToolBar")->setToolButtonStyle (Qt::ToolButtonIconOnly);
    adjustSize ();

    // Save GUI settings.
    setAutoSaveSettings ();

    // Start the game with a randomised demo.
    game->initGame (gameView, this);
}


Kubrick::~Kubrick ()
{
}


const Kubrick::PuzzleItem Kubrick::easyItems [] = {
    {I18N_NOOP("2x2x1 mat, 1 move"),    2, 2, 1, 1, 1, 1},
    {I18N_NOOP("2x2x1 mat, 2 moves"),   2, 2, 1, 2, 1, 1},
    {I18N_NOOP("2x2x1 mat, 3 moves"),   2, 2, 1, 3, 1, 1},
    {I18N_NOOP("2x2x2 cube, 2 moves"),  2, 2, 2, 2, 1, 1},
    {I18N_NOOP("2x2x2 cube, 3 moves"),  2, 2, 2, 3, 1, 1},
    {I18N_NOOP("2x2x2 cube, 4 moves"),  2, 2, 2, 4, 1, 0},
    {I18N_NOOP("3x3x1 mat, 4 moves"),   3, 3, 1, 4, 1, 0},
    {I18N_NOOP("3x3x3 cube, 3 moves"),  3, 3, 3, 3, 1, 0},
    {I18N_NOOP("3x3x3 cube, 4 moves"),  3, 3, 3, 4, 1, 0},
    {"END",				0, 0, 0, 0, 0, 0}
};


const Kubrick::PuzzleItem Kubrick::notSoEasyItems [] = {
    {I18N_NOOP("3x3x3 cube, 3 moves"),  3, 3, 3, 3, 0, 0},
    {I18N_NOOP("3x3x3 cube, 4 moves"),  3, 3, 3, 4, 0, 0},
    {I18N_NOOP("4x4x4 cube, 4 moves"),  4, 4, 4, 4, 0, 0},
    {I18N_NOOP("5x5x5 cube, 4 moves"),  5, 5, 5, 4, 0, 0},
    {I18N_NOOP("6x3x2 brick, 4 moves"), 6, 3, 2, 4, 0, 0},
    {"END",				0, 0, 0, 0, 0, 0}
};


const Kubrick::PuzzleItem Kubrick::hardItems [] = {
    {I18N_NOOP("3x3x3 cube, 7 moves"),  3, 3, 3, 7, 0, 0},
    {I18N_NOOP("4x4x4 cube, 5 moves"),  4, 4, 4, 5, 0, 0},
    {I18N_NOOP("5x5x5 cube, 6 moves"),  5, 5, 5, 6, 0, 0},
    {I18N_NOOP("6x6x6 cube, 6 moves"),  6, 6, 6, 6, 0, 0},
    {I18N_NOOP("6x4x1 mat, 9 moves"),   6, 4, 1, 9, 0, 0},
    {I18N_NOOP("6x3x2 brick, 6 moves"), 6, 3, 2, 7, 0, 0},
    {"END",				0, 0, 0, 0, 0, 0}
};


const Kubrick::PuzzleItem Kubrick::veryHardItems [] = {
    {I18N_NOOP("3x3x3 cube, 12 moves"), 3, 3, 3, 12, 0, 0},
    {I18N_NOOP("3x3x3 cube, 15 moves"), 3, 3, 3, 15, 0, 0},
    {I18N_NOOP("3x3x3 cube, 20 moves"), 3, 3, 3, 20, 0, 0},
    {I18N_NOOP("4x4x4 cube, 12 moves"), 4, 4, 4, 12, 0, 0},
    {I18N_NOOP("5x5x5 cube, 15 moves"), 5, 5, 5, 15, 0, 0},
    {I18N_NOOP("6x6x6 cube, 25 moves"), 6, 6, 6, 25, 0, 0},
    {"END",				0, 0, 0,  0, 0, 0}
};


const QString Kubrick::patternMovesInfo = I18N_NOOP(
    "Rubik's Cube can be moved into many interesting patterns.  Here are "
    "a few from David Singmaster's classic book 'Notes on Rubik's Magic Cube, "
    "Fifth Edition', pages 47-49, published in 1981.  After a pattern has "
    "formed, you can use the Solve action (default key S) to undo and redo it "
    "as often as you like."
    );


const Kubrick::DemoItem Kubrick::patterns [] = {
    {"",			I18N_NOOP("Info")},
    {"p333X6.kbk",		I18N_NOOP("3x3x3, 6 X")},
    {"p333X2.kbk",		I18N_NOOP("3x3x3, 2 X")},
    {"p333Spot6.kbk",		I18N_NOOP("3x3x3, 6 Spot")},
    {"p333Spot4.kbk",		I18N_NOOP("3x3x3, 4 Spot")},
    {"p333Plus4.kbk",		I18N_NOOP("3x3x3, 4 Plus")},
    {"p333Bar4.kbk",		I18N_NOOP("3x3x3, 4 Bar")},
    {"p333U6.kbk",		I18N_NOOP("3x3x3, 6 U")},
    {"p333U4.kbk",		I18N_NOOP("3x3x3, 4 U")},
    {"p333Snake.kbk",		I18N_NOOP("3x3x3, Snake")},
    {"p333Worm.kbk",		I18N_NOOP("3x3x3, Worm")},
    {"p333Tricolor6.kbk",	I18N_NOOP("3x3x3, Tricolor")},
    {"p333DoubleCube.kbk",	I18N_NOOP("3x3x3, Double Cube")},
    {"END",			""}
};


const QString Kubrick::solvingMovesInfo = I18N_NOOP(
    "<qt>Mathematicians calculate that a 3x3x3 cube can be shuffled into "
    "43,252,003,274,489,856,000 different patterns, yet they conjecture "
    "that all positions can be solved in 20 moves or less.  The method "
    "that can do that (as yet undiscovered) is called God's Algorithm."
    "<br><br>"
    "Many longer methods are known (see <a href=en.wikipedia.org>Optimal "
    "Solutions for Rubik's Cube at en.wikipedia.org</a>)."
    "<br><br>"
    "Several methods work systematically by building the solution one layer "
    "at a time, using sequences of moves that solve a few pieces without "
    "disturbing what has already been done.  The 'Beginner Solution' "
    "demonstrated here is from "
    "<a href=www.geocities.com/jasmine_ellen/RubiksCubeSolution.html>"
    "www.geocities.com/jasmine_ellen/RubiksCubeSolution.html</a> (see that "
    "site for a full description and other solution ideas).  Just over "
    "100 moves solve a cube that is shuffled in 20.</qt>"
    );
// Setup the GUI, menus, ...
const Kubrick::DemoItem Kubrick::solvingMoves [] = {
    {"",		    I18N_NOOP("Info")},
    {"m333Layer1.kbk",	    I18N_NOOP("3x3x3 Layer 1, Edges First")},
    {"m333MEdge1.kbk",	    I18N_NOOP("3x3x3 Layer 2, Edge from Bottom Right")},
    {"m333MEdge2.kbk",	    I18N_NOOP("3x3x3 Layer 2, Edge from Bottom Left")},
    {"m333LLEdgeFlip.kbk",  I18N_NOOP("3x3x3 Layer 3, Flip Edge Pieces")},
    {"m333LLCornerPos.kbk", I18N_NOOP("3x3x3 Layer 3, Place Corners")},
    {"m333LLCornerRot.kbk", I18N_NOOP("3x3x3 Layer 3, Twist Corners")},
    {"m333LLEdgePos.kbk",   I18N_NOOP("3x3x3 Layer 3, Place Edges and DONE!")},
    {"m333Complete.kbk",    I18N_NOOP("3x3x3 Cube, Complete Solution")},
    {"m333E2prX.kbk",	    I18N_NOOP("3x3x3 Swap 2 Pairs of Edges")},
    {"m333CTwirl2.kbk",	    I18N_NOOP("3x3x3 Untwist 2 Corners")},
    {"m333EFlip2.kbk",	    I18N_NOOP("3x3x3 Flip 2 Edges")},
    {"END",		    ""}
};


// Create an action and set some help text for all menu items
// See also the *ui.rc file for the XML definitions
void Kubrick::initGUI()
{
    // Game menu.
    QAction * newAction =	KStandardGameAction::gameNew (
				game, SLOT (newPuzzle()), this);
    actionCollection()->addAction (newAction->objectName(), newAction);
    newAction->setText		(i18n("&New Puzzle"));
    newAction->setToolTip	(i18n("Start a new puzzle."));
    newAction->setWhatsThis	(i18n("Finish the puzzle you are working on "
				"and start a new puzzle with the same "
				"dimensions and number of shuffling moves."));

    QAction *
    a =				KStandardGameAction::load (
				game, SLOT (load()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setText			(i18n("&Load Puzzle..."));
    a->setToolTip		(i18n("Reload a saved puzzle from a file."));
    a->setWhatsThis		(i18n("Reload a puzzle you have previously "
				"saved on a file, including its dimensions, "
				"settings, current state and history of "
				"moves."));

    a =				KStandardGameAction::save (
				game, SLOT (save()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setText			(i18n("&Save Puzzle..."));
    a->setToolTip		(i18n("Save the puzzle on a file."));
    a->setWhatsThis		(i18n("Save the puzzle on a file, including "
				"its dimensions, settings, current state and "
				"history of moves."));

    a =				KStandardGameAction::saveAs (
				game, SLOT (saveAs()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setText			(i18n("&Save Puzzle As..."));

    a =				KStandardGameAction::
				restart (game, SLOT (undoAll()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setText			(i18n("Restart &Puzzle..."));
    a->setToolTip		(i18n("Undo all previous moves and start "
				"again."));
    a->setWhatsThis		(i18n("Undo all previous moves and start "
				"again."));

    a =				KStandardGameAction::
				quit (this, SLOT (close()), this);
    actionCollection()->addAction (a->objectName(), a);
    // NOTE: KXmlGuiWindow::close() calls Kubrick::queryClose(), our real "quit"

    // Move menu.
    a =				KStandardGameAction::
				undo (game, SLOT (undoMove()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setToolTip		(i18n("Undo the last move."));
    a->setWhatsThis		(i18n("Undo the last move."));

    a =				KStandardGameAction::
				redo (game, SLOT (redoMove()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setToolTip		(i18n("Redo a previously undone move."));
    a->setWhatsThis		(i18n("Redo a previously undone move "
				"(repeatedly from the start if required)."));

    a =				KStandardGameAction::
				solve (game, SLOT (solveCube()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setToolTip		(i18n("Show the solution of the puzzle."));
    a->setWhatsThis		(i18n("Show the solution of the puzzle by "
				"undoing and re-doing all shuffling moves."));

    a =				KStandardGameAction::
				demo (game, SLOT (toggleDemo()), this);
    actionCollection()->addAction (a->objectName(), a);
    a->setText			(i18n("Main &Demo"));
    a->setToolTip		(i18n("Run a demonstration of puzzle moves."));
    a->setWhatsThis		(i18n("Run a demonstration of puzzle moves, "
				"in which randomly chosen cubes, bricks or "
				"mats are shuffled and solved."));

    a = actionCollection()->addAction ("standard_view");
    a->setText			(i18n("Realign Cube"));
    a->setToolTip		(i18n("Realign the cube so that the top, "
				"front and right faces are visible together."));
    a->setWhatsThis		(i18n("Realign the cube so that the top, "
				"front and right faces are visible together "
				"and the cube's axes are parallel to the XYZ "
				"axes, thus making keyboard moves properly "
				"meaningful."));
    a->setIcon			(KIcon("go-home"));
    a->setShortcut		(Qt::Key_Home);
    connect (a, SIGNAL (triggered (bool)), game, SLOT (setStandardView()));

    a = actionCollection()->addAction ("redo_all");
    a->setText (i18n("Redo All"));
    a->setShortcut (Qt::SHIFT + Qt::Key_R);
    connect (a, SIGNAL (triggered (bool)), game, SLOT (redoAll()));

    QWidgetAction * w = new QWidgetAction (this);
    actionCollection()->addAction ("singmaster_label", w);
    QLabel * singmasterLabel = new QLabel ("&Singmaster Moves", this);
    w->setDefaultWidget (singmasterLabel);

    w = new QWidgetAction (this);
    actionCollection()->addAction ("singmaster_move", w);
    w->setText			(i18n("Singmaster Moves"));
    w->setToolTip		(i18n("You can enter Singmaster moves here"));
    w->setWhatsThis		(i18n("xxxxxxxxxxxxxx"));
    // connect (w, SIGNAL (triggered (bool)), game, SLOT (TBD IDW()));
    QLineEdit * singmasterMoves = new QLineEdit (this);
    w->setDefaultWidget (singmasterMoves);
    singmasterLabel->setBuddy (singmasterMoves);
    singmasterMoves->show();
    singmasterLabel->show();
    singmasterMoves->setFocusPolicy (Qt::ClickFocus);
    singmasterMoves->clearFocus();

    // "Choose Puzzle Type" sub-menu.
    easyList = new KSelectAction (i18n("&Easy"), this);
    actionCollection()->addAction ("easy_list", easyList);
    fillPuzzleList (easyList, easyItems);
    connect (easyList, SIGNAL(triggered(int)), SLOT(easySelected(int)));

    notSoEasyList = new KSelectAction (i18n("&Not So Easy"), this);
    actionCollection()->addAction ("not_easy_list", notSoEasyList);
    fillPuzzleList (notSoEasyList, notSoEasyItems);
    connect (notSoEasyList,SIGNAL(triggered(int)),SLOT(notSoEasySelected(int)));

    hardList = new KSelectAction (i18n("&Hard"), this);
    actionCollection()->addAction ("hard_list", hardList);
    fillPuzzleList (hardList, hardItems);
    connect (hardList, SIGNAL(triggered(int)), SLOT(hardSelected(int)));

    veryHardList = new KSelectAction (i18n("&Very Hard"), this);
    actionCollection()->addAction ("very_hard_list", veryHardList);
    fillPuzzleList (veryHardList, veryHardItems);
    connect (veryHardList, SIGNAL(triggered(int)), SLOT(veryHardSelected(int)));

    a = actionCollection()->addAction ("new_cube");
    a->setText (i18n("Make your own..."));
    connect (a, SIGNAL (triggered (bool)), game, SLOT (newCubeDialog()));

    // View menu.
    KToggleAction * b;
    QActionGroup * viewGroup = new QActionGroup (this);
    viewGroup->setExclusive (true);

    QSignalMapper * viewMapper = new QSignalMapper (this);
    connect (viewMapper, SIGNAL (mapped (const int)),
                game, SLOT (changeScene (const int)));

    b = new KToggleAction	(i18n ("1 Cube"), this);
    actionCollection()->addAction ("scene_1", b);
    b->setToolTip		(i18n ("Show one view of this cube."));
    b->setWhatsThis		(i18n ("Show one view of this cube, "
				"from the front."));
    b->setIcon			(KIcon("arrow-left")); // IDW - Temporary.
    connect (b, SIGNAL(triggered (bool)), viewMapper, SLOT(map()));
    b->setChecked (true);
    viewMapper->setMapping (b, OneCube);
    viewGroup->addAction (b);

    b = new KToggleAction	(i18n ("2 Cubes"), this);
    actionCollection()->addAction ("scene_2", b);
    b->setToolTip		(i18n ("Show two views of this cube."));
    b->setWhatsThis		(i18n ("Show two views of this cube, from "
				"the front and the back.  Both can rotate."));
    b->setIcon			(KIcon("arrow-up")); // IDW - Temporary.
    connect (b, SIGNAL(triggered (bool)), viewMapper, SLOT(map()));
    viewMapper->setMapping (b, TwoCubes);
    viewGroup->addAction (b);

    b = new KToggleAction	(i18n ("3 Cubes"), this);
    actionCollection()->addAction ("scene_3", b);
    b->setToolTip		(i18n ("Show three views of this cube."));
    b->setWhatsThis		(i18n ("Show three views of this cube, a "
				"large one, from the front, and two small "
				"ones, from the front and the back.  Only "
				"the large one can rotate."));
    b->setIcon			 (KIcon("arrow-right")); // IDW - Temporary.
    connect (b, SIGNAL(triggered (bool)), viewMapper, SLOT(map()));
    viewMapper->setMapping (b, ThreeCubes);
    viewGroup->addAction (b);

    // Demos menu.
    patternList = new KSelectAction (i18n("&Pretty Patterns"), this);
    actionCollection()->addAction ("patterns_list", patternList);
    maxPatternsIndex = fillDemoList (patternList, patterns);
    connect (patternList, SIGNAL(triggered(int)), SLOT(patternSelected(int)));

    movesList = new KSelectAction (i18n("&Solution Moves"), this);
    actionCollection()->addAction ("demo_moves_list", movesList);
    maxMovesIndex =    fillDemoList (movesList, solvingMoves);
    connect (movesList, SIGNAL(triggered(int)), SLOT(movesSelected(int)));

    // Settings menu.
    b = new KToggleAction (i18n("&Watch Shuffling"), this);
    actionCollection()->addAction ("watch_shuffling", b);
    b->setShortcut (Qt::Key_W);
    connect (b, SIGNAL (triggered (bool)), game, SLOT (watchShuffling()));

    b = new KToggleAction (i18n("Watch Your &Own Moves"), this);
    actionCollection()->addAction ("watch_moves", b);
    b->setShortcut (Qt::Key_O);
    connect (b, SIGNAL (triggered (bool)), game, SLOT (watchMoves()));

    // DISCONTINUED a = actionCollection()->addAction ("enable_messages");
    // a->setText (i18n("Show Beginners' &Messages"));
    // connect (a, SIGNAL (triggered (bool)), game, SLOT (enableMessages()));

    // Sorry to be "non-standard" (below), but I am an English-speaker, born and
    // bred, and I just don't find the KDE standard texts "Configure Kubrick"
    // and "Configure Shortcuts" to be meaningful in everyday language.  Neither
    // would my wife, children and friends, I am sure.

    a = KStandardAction::preferences (game, SLOT(optionsDialog()),
                                      actionCollection());
    a->setText (i18n("Kubri&ck Game Settings"));

    // Configure Shortcuts...
    a = KStandardAction::keyBindings (this, SLOT(optionsConfigureKeys()),
                                      actionCollection());
    a->setText (i18n("Keyboard S&hortcut Settings"));

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Keys to choose the axis for a slice move (X, Y or Z).
    QSignalMapper * moveAxis = new QSignalMapper (this);

    a = actionCollection()->addAction ("x_axis");
    a->setText (i18n("X Axis"));
    a->setShortcut (Qt::Key_X);
    connect (a, SIGNAL (triggered (bool)), moveAxis, SLOT (map()));
    moveAxis->setMapping (a, 0);

    a = actionCollection()->addAction ("y_axis");
    a->setText (i18n("Y Axis"));
    a->setShortcut (Qt::Key_Y);
    connect (a, SIGNAL (triggered (bool)), moveAxis, SLOT (map()));
    moveAxis->setMapping (a, 1);

    a = actionCollection()->addAction ("z_axis");
    a->setText (i18n("Z Axis"));
    a->setShortcut (Qt::Key_Z);
    connect (a, SIGNAL (triggered (bool)), moveAxis, SLOT (map()));
    moveAxis->setMapping (a, 2);

    connect (moveAxis, SIGNAL (mapped (int)), game, SLOT (setMoveAxis (int)));

    // Keys to choose the slice number for a slice move.
    QSignalMapper * moveSlice = new QSignalMapper (this);

    char ident [10];
    strcpy (ident, "slice n");
    for (int i = 1; i <= 6; i++) {
        sprintf (ident, "slice %d", i);
        a = actionCollection()->addAction (ident);
        a->setText (i18n("Slice %1", i));
        a->setShortcut (Qt::Key_0 + i);
        connect (a, SIGNAL (triggered (bool)), moveSlice, SLOT (map()));
        moveSlice->setMapping (a, i);
    }

    // Key to select a rotation of the whole cube (mapped as "slice 0").
    a = actionCollection()->addAction ("turn_cube");
    a->setText (i18n("Turn whole cube"));
    a->setShortcut (Qt::Key_C);
    connect (a, SIGNAL (triggered (bool)), moveSlice, SLOT (map()));
    moveSlice->setMapping (a, 0);

    connect (moveSlice, SIGNAL (mapped (int)), game, SLOT (setMoveSlice(int)));

    // Keys to choose the direction for a slice move (clock or anti-clock).
    QSignalMapper * moveDirection = new QSignalMapper (this);

    a = actionCollection()->addAction ("anti_clockwise");
    a->setText (i18n("Anti-clockwise"));
    a->setShortcut (Qt::Key_Left);
    connect (a, SIGNAL (triggered (bool)), moveDirection, SLOT (map()));
    moveDirection->setMapping (a, 0);

    a = actionCollection()->addAction ("clockwise");
    a->setText (i18n("Clockwise"));
    a->setShortcut (Qt::Key_Right);
    connect (a, SIGNAL (triggered (bool)), moveDirection, SLOT (map()));
    moveDirection->setMapping (a, 1);

    connect (moveDirection, SIGNAL (mapped (int)),
                game, SLOT (setMoveDirection (int)));

    // Keys for Singmaster (sm) moves.
    QSignalMapper * smMove = new QSignalMapper (this);
    a = mapAction (smMove, "sm_u", i18n("Move 'Up' face"),
					Qt::Key_U, SM_UP);
    a = mapAction (smMove, "sm_d", i18n("Move 'Down' face"),
					Qt::Key_E, SM_DOWN);
					// IDW Qt::Key_D, SM_DOWN);
    a = mapAction (smMove, "sm_l", i18n("Move 'Left' face"),
					Qt::Key_L, SM_LEFT);
    a = mapAction (smMove, "sm_r", i18n("Move 'Right' face"),
					Qt::Key_R, SM_RIGHT);
    a = mapAction (smMove, "sm_f", i18n("Move 'Front' face"),
					Qt::Key_F, SM_FRONT);
    a = mapAction (smMove, "sm_b", i18n("Move 'Back' face"),
					Qt::Key_B, SM_BACK);
    a = mapAction (smMove, "sm_anti", i18n("Anti-clockwise move"),
					Qt::Key_Apostrophe, SM_ANTICLOCKWISE);
    a = mapAction (smMove, "sm_plus", i18n("Singmaster two-slice move"),
					Qt::Key_Plus, SM_2_SLICE);
    a = mapAction (smMove, "sm_minus", i18n("Singmaster anti-slice move"),
					Qt::Key_Minus, SM_ANTISLICE);
    a = mapAction (smMove, "sm_dot", i18n("Move an inner slice"),
					Qt::Key_Period, SM_INNER);
    a = mapAction (smMove, "sm_exec", i18n("Complete a Singmaster sequence"),
					Qt::Key_Return, SM_EXECUTE);
    connect (smMove, SIGNAL (mapped (int)), game, SLOT (smInput (int)));
}


QAction * Kubrick::mapAction (QSignalMapper * mapper, const QString & name,
		const QString & text, const Qt::Key key, SingmasterMove mapping)
{
    QAction * a;
    a = actionCollection()->addAction (name);
    kDebug() << name << text << key << mapping;
    a->setText (text);
    a->setShortcut (key);
    connect (a, SIGNAL (triggered (bool)), mapper, SLOT (map()));
    mapper->setMapping (a, mapping);
    return a;
}


void Kubrick::setToggle (const char * actionName, bool onOff)
{
    ((KToggleAction *) ACTION (actionName))->setChecked (onOff);
}


void Kubrick::setAvail (const char * actionName, bool onOff)
{
    ((KAction *) ACTION (actionName))->setEnabled (onOff);
}


int Kubrick::fillPuzzleList (KSelectAction * s, const PuzzleItem itemList [])
{
    QStringList list;

    for (uint i=0; (strcmp (itemList[i].menuText, "END") != 0); i++) {
	list.append (i18n(itemList[i].menuText));
    }
    list.append (".");		// Add dummy item to hold unwanted checkbox.
    s->setItems(list);
    return (list.count() - 1);
}


int Kubrick::fillDemoList (KSelectAction * s, const DemoItem itemList [])
{
    QStringList list;

    for (uint i=0; (strcmp (itemList[i].filename, "END") != 0); i++) {
	list.append (i18n(itemList[i].menuText));
    }
    list.append (".");		// Add dummy item to hold unwanted checkbox.
    s->setItems(list);
    return (list.count() - 1);
}


void Kubrick::easySelected (int index)
{
    statusBar()->changeItem (easyItems [index].menuText, 1001);
    game->changePuzzle (easyItems [index]);
}


void Kubrick::notSoEasySelected (int index)
{
    statusBar()->changeItem (notSoEasyItems [index].menuText, 1001);
    game->changePuzzle (notSoEasyItems [index]);
}


void Kubrick::hardSelected (int index)
{
    statusBar()->changeItem (hardItems [index].menuText, 1001);
    game->changePuzzle (hardItems [index]);
}


void Kubrick::veryHardSelected (int index)
{
    statusBar()->changeItem (veryHardItems [index].menuText, 1001);
    game->changePuzzle (veryHardItems [index]);
}


void Kubrick::patternSelected (int index)
{
    kDebug() << "Pattern" << index;
    if (index > 0) {
	game->loadDemo (patterns[index].filename);
	statusBar()->changeItem (patterns[index].menuText, 1001);
    }
    else {
	KMessageBox::information (this,
		patternMovesInfo,
		i18n("Pretty Patterns"));
    }
    // Kludge to make the unwanted KSelectAction checkbox go onto a dummy item.
    patternList->setCurrentItem (maxPatternsIndex);
}


void Kubrick::movesSelected (int index)
{
    if (index > 0) {
	game->loadDemo (solvingMoves[index].filename);
	statusBar()->changeItem (solvingMoves[index].menuText, 1001);
    }
    else {
	KMessageBox::information (this,
		solvingMovesInfo,
		i18n("Solution Moves"));
    }
    // Kludge to make the unwanted KSelectAction checkbox go onto a dummy item.
    movesList->setCurrentItem (maxMovesIndex);
}


void Kubrick::describePuzzle (int xDim, int yDim, int zDim, int shMoves)
{
    QString descr;
    if ((xDim == yDim) && (yDim == zDim)) {
	descr = i18n ("%1x%2x%3 cube, %4 moves", xDim, yDim, zDim, shMoves);
    }
    else if ((xDim != 1) && (yDim != 1) && (zDim != 1)) {
	descr = i18n ("%1x%2x%3 brick, %4 moves", xDim, yDim, zDim, shMoves);
    }
    else {
	descr = i18n ("%1x%2x%3 mat, %4 moves", xDim, yDim, zDim, shMoves);
    }
    statusBar()->changeItem (descr, 1001);
}


void Kubrick::optionsConfigureKeys()
{
    KShortcutsDialog::configure(actionCollection());
}


// This method is invoked by KXmlGuiWindow when the window is closed, whether by
// selecting the "Quit" action or by clicking the "X" widget at the top right.

bool Kubrick::queryClose ()
{
    game->saveState ();         	// Save the current state of the cube.
    return (true);
}

#include "kubrick.moc"
