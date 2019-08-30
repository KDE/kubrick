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

// Own header
#include "kubrick.h"

// Qt includes.
#include <QAction>
#include <QGLFormat>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QString>

// KDE includes.
#include <KActionCollection>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShortcutsDialog>
#include <KStandardGameAction>
#include <KToggleAction>

// Local includes.
#include "game.h"
#include "gameglview.h"

// Shorthand for reference to actions.
#define ACTION(x)   (actionCollection()->action(x))

Kubrick::Kubrick () :
    singmasterMoves (nullptr)
{
    // Window title.
    // setWindowTitle("Rubik's Cube");	// DELETED - This is a *trademark*.

    // use multi-sample (anti-aliased) OpenGL if available
    /* Disable it for now, was causing black screens in some configurations
    QGLFormat defFormat = QGLFormat::defaultFormat();
    defFormat.setSampleBuffers(true);
    QGLFormat::setDefaultFormat(defFormat);
    */

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

     // Demos menu. This needs to be after setupGUI() call.
    auto slot = &Kubrick::patternSelected;
    Q_UNUSED(slot)
    fillDemoList (patterns, patternList,
			"patterns_list", &Kubrick::patternSelected);
    fillDemoList (solvingMoves, movesList,
			"demo_moves_list", &Kubrick::movesSelected);

    // Set up a status bar.
    statusBar()->show ();
    statusBarLabel = new QLabel(i18n("Welcome to Kubrick"), this);
    statusBarLabel->setAlignment (Qt::AlignCenter);
    statusBar()->addWidget (statusBarLabel, 1);

    // Set a larger font than toolbar-default for the Singmaster-moves display.
    QFont f = statusBar()->font();
    f.setPointSize (f.pointSize());	// Needed to force a size-change.
    singmasterLabel->setFont (f);
    singmasterMoves->setFont (f);

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


const char * Kubrick::patternMovesInfo = I18N_NOOP(
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


const char * Kubrick::solvingMovesInfo = I18N_NOOP(
    "<qt>Mathematicians calculate that a 3x3x3 cube can be shuffled into "
    "43,252,003,274,489,856,000 different patterns, yet they conjecture "
    "that all positions can be solved in 20 moves or less.  The method "
    "that can do that (as yet undiscovered) is called God's Algorithm."
    "<br><br>"
    "Many longer methods are known.  See the two Wikipedia articles on "
    "Rubik's Cube and Optimal Solutions for Rubik's Cube."
    "<br><br>"
    "Several methods work systematically by building the solution one layer "
    "at a time, using sequences of moves that solve a few pieces without "
    "disturbing what has already been done.  The 'Beginner Solution' "
    "demonstrated here uses that approach.  Just over 100 moves solve a cube "
    "that is shuffled in 20.</qt>"
    );


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

    a = actionCollection()->addAction ( QStringLiteral( "standard_view" ));
    a->setText			(i18n("Realign Cube"));
    a->setToolTip		(i18n("Realign the cube so that the top, "
				"front and right faces are visible together."));
    a->setWhatsThis		(i18n("Realign the cube so that the top, "
				"front and right faces are visible together "
				"and the cube's axes are parallel to the XYZ "
				"axes, thus making keyboard moves properly "
				"meaningful."));
    a->setIcon			(QIcon::fromTheme( QStringLiteral( "go-home" )));
    actionCollection()->setDefaultShortcut(a, Qt::Key_Home);
    connect (a, &QAction::triggered, game, &Game::setStandardView);

    a = actionCollection()->addAction ( QStringLiteral( "redo_all" ));
    a->setText (i18n("Redo All"));
    actionCollection()->setDefaultShortcut(a, Qt::SHIFT + Qt::Key_R);
    connect (a, &QAction::triggered, game, &Game::redoAll);

    // Read-only display of Singmaster moves on the toolbar.
    singmasterLabel = new QLabel (i18n("Singmaster Moves"), this);
    singmasterMoves = new QLineEdit (this);

    QWidget *sigmasterWidget = new QWidget(this);
    QHBoxLayout *sigmasterLayout = new QHBoxLayout (sigmasterWidget);
    sigmasterLayout->setContentsMargins(0, 0, 0, 0);
    sigmasterLayout->addWidget(singmasterLabel);
    sigmasterLayout->addWidget(singmasterMoves);

    QWidgetAction *w = new QWidgetAction (this);
    actionCollection()->addAction ( QStringLiteral( "singmaster_label" ), w);
    w->setDefaultWidget (sigmasterWidget);

    actionCollection()->addAction ( QStringLiteral( "singmaster_moves" ), w);

    QString singmasterToolTip = i18n("This area shows Singmaster moves.");
    QString singmasterWhatsThis = i18nc("The letters RLFBUD are mathematical "
			"notation based on English words. Please leave those "
			"letters and words untranslated in some form.",

			"This area shows Singmaster moves. "
			"They are based on the letters RLFBUD, representing "
			"(in English) the Right, Left, Front, Back, Up and "
			"Down faces. In normal view, the letters RFU represent "
			"clockwise moves of the three visible faces and LBD "
			"appear as anticlockwise moves of the hidden faces. "
			"Adding a ' (apostrophe) to a letter gives the reverse "
			"of that letter's move. To move inner slices, add "
			"periods (or dots) before the letter of the nearest "
			"face.");

    singmasterLabel->setToolTip	(singmasterToolTip);
    singmasterLabel->setWhatsThis (singmasterWhatsThis);
    singmasterMoves->setToolTip	(singmasterToolTip);
    singmasterMoves->setWhatsThis (singmasterWhatsThis);

    singmasterMoves->setReadOnly (true);
    singmasterLabel->setBuddy (singmasterMoves);
    singmasterMoves->show();
    singmasterLabel->show();
    singmasterMoves->setFocusPolicy (Qt::NoFocus);
    singmasterMoves->clearFocus();

    // "Choose Puzzle Type" sub-menu.
    easyList = new KSelectAction (i18n("&Easy"), this);
    actionCollection()->addAction ( QStringLiteral( "easy_list" ), easyList);
    fillPuzzleList (easyList, easyItems);
    connect(easyList, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Kubrick::easySelected);

    notSoEasyList = new KSelectAction (i18n("&Not So Easy"), this);
    actionCollection()->addAction ( QStringLiteral( "not_easy_list" ), notSoEasyList);
    fillPuzzleList (notSoEasyList, notSoEasyItems);
    connect(notSoEasyList, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Kubrick::notSoEasySelected);

    hardList = new KSelectAction (i18n("&Hard"), this);
    actionCollection()->addAction ( QStringLiteral( "hard_list" ), hardList);
    fillPuzzleList (hardList, hardItems);
    connect(hardList, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Kubrick::hardSelected);

    veryHardList = new KSelectAction (i18n("&Very Hard"), this);
    actionCollection()->addAction ( QStringLiteral( "very_hard_list" ), veryHardList);
    fillPuzzleList (veryHardList, veryHardItems);
    connect(veryHardList, static_cast<void (KSelectAction::*)(int)>(&KSelectAction::triggered), this, &Kubrick::veryHardSelected);

    a = actionCollection()->addAction ( QStringLiteral( "new_cube" ));
    a->setText (i18n("Make your own..."));
    connect (a, &QAction::triggered, game, &Game::newCubeDialog);

    // View menu.
    KToggleAction * b;
    QActionGroup * viewGroup = new QActionGroup (this);
    viewGroup->setExclusive (true);

    b = new KToggleAction	(i18n ("1 Cube"), this);
    actionCollection()->addAction ( QStringLiteral( "scene_1" ), b);
    b->setToolTip		(i18n ("Show one view of this cube."));
    b->setWhatsThis		(i18n ("Show one view of this cube, "
				"from the front."));
    b->setIcon			(QIcon::fromTheme( QStringLiteral( "arrow-left" ))); // IDW - Temporary.
    connect(b, &KToggleAction::triggered, game, [this] { game->changeScene(OneCube); });
    b->setChecked (true);
    viewGroup->addAction (b);

    b = new KToggleAction	(i18n ("2 Cubes"), this);
    actionCollection()->addAction ( QStringLiteral( "scene_2" ), b);
    b->setToolTip		(i18n ("Show two views of this cube."));
    b->setWhatsThis		(i18n ("Show two views of this cube, from "
				"the front and the back.  Both can rotate."));
    b->setIcon			(QIcon::fromTheme( QStringLiteral( "arrow-up" ))); // IDW - Temporary.
    connect(b, &KToggleAction::triggered, game, [this] { game->changeScene(TwoCubes); });
    viewGroup->addAction (b);

    b = new KToggleAction	(i18n ("3 Cubes"), this);
    actionCollection()->addAction ( QStringLiteral( "scene_3" ), b);
    b->setToolTip		(i18n ("Show three views of this cube."));
    b->setWhatsThis		(i18n ("Show three views of this cube, a "
				"large one, from the front, and two small "
				"ones, from the front and the back.  Only "
				"the large one can rotate."));
    b->setIcon			 (QIcon::fromTheme( QStringLiteral( "arrow-right" ))); // IDW - Temporary.
    connect(b, &KToggleAction::triggered, game, [this] { game->changeScene(ThreeCubes); });
    viewGroup->addAction (b);

    // Demos menu.  See the code after "setupGUI ();".

    // Settings menu.
    b = new KToggleAction (i18n("&Watch Shuffling"), this);
    actionCollection()->addAction ( QStringLiteral( "watch_shuffling" ), b);
    actionCollection()->setDefaultShortcut(b, Qt::Key_W);
    connect(b, &KToggleAction::triggered, game, &Game::watchShuffling);

    b = new KToggleAction (i18n("Watch Your &Own Moves"), this);
    actionCollection()->addAction ( QStringLiteral( "watch_moves" ), b);
    actionCollection()->setDefaultShortcut(b, Qt::Key_O);
    connect(b, &KToggleAction::triggered, game, &Game::watchMoves);

    // DISCONTINUED a = actionCollection()->addAction ( QLatin1String( "enable_messages" ));
    // a->setText (i18n("Show Beginners' &Messages"));
    // connect (a, &QAction::triggered, game, &Game::enableMessages);

    // Sorry to be "non-standard" (below), but I am an English-speaker, born and
    // bred, and I just don't find the KDE standard texts "Configure Kubrick"
    // and "Configure Shortcuts" to be meaningful in everyday language.  Neither
    // would my wife, children and friends, I am sure.

    a = KStandardAction::preferences (game, &Game::optionsDialog, actionCollection());
    a->setText (i18n("Kubri&ck Game Settings"));

    // Configure Shortcuts...
    a = KStandardAction::keyBindings (this, &Kubrick::optionsConfigureKeys, actionCollection());
    a->setText (i18n("Keyboard S&hortcut Settings"));

    /**************************************************************************/
    /**************************   KEYSTROKE ACTIONS  **************************/
    /**************************************************************************/

    // Keys to choose the axis for a slice move (X, Y or Z).
    a = actionCollection()->addAction ( QStringLiteral( "x_axis" ));
    a->setText (i18n("X Axis"));
    actionCollection()->setDefaultShortcut(a, Qt::Key_X);
    connect(a, &QAction::triggered, game, [this] { game->setMoveAxis(0); });

    a = actionCollection()->addAction ( QStringLiteral( "y_axis" ));
    a->setText (i18n("Y Axis"));
    actionCollection()->setDefaultShortcut(a,Qt::Key_Y);
    connect(a, &QAction::triggered, game, [this] { game->setMoveAxis(1); });

    a = actionCollection()->addAction ( QStringLiteral( "z_axis" ));
    a->setText (i18n("Z Axis"));
    actionCollection()->setDefaultShortcut(a,Qt::Key_Z);
    connect(a, &QAction::triggered, game, [this] { game->setMoveAxis(2); });

    // Keys to choose the slice number for a slice move.
    char ident [10];
    strcpy (ident, "slice n");
    for (int i = 1; i <= 6; i++) {
        sprintf (ident, "slice %d", i);
        a = actionCollection()->addAction (ident);
        a->setText (i18n("Slice %1", i));
        actionCollection()->setDefaultShortcut(a, Qt::Key_0 + i);
        connect(a, &QAction::triggered, game, [this, i] { game->setMoveSlice(i); });
    }

    // Key to select a rotation of the whole cube (mapped as "slice 0").
    a = actionCollection()->addAction ( QStringLiteral( "turn_cube" ));
    a->setText (i18n("Turn whole cube"));
    actionCollection()->setDefaultShortcut(a,Qt::Key_C);
    connect(a, &QAction::triggered, game, [this] { game->setMoveSlice(0); });

    // Keys to choose the direction for a slice move (clock or anti-clock).
    a = actionCollection()->addAction ( QStringLiteral( "anti_clockwise" ));
    a->setText (i18n("Anti-clockwise"));
    actionCollection()->setDefaultShortcut(a,Qt::Key_Left);
    connect (a, &QAction::triggered, game, [this] { game->setMoveDirection(0); });

    a = actionCollection()->addAction ( QStringLiteral( "clockwise" ));
    a->setText (i18n("Clockwise"));
    actionCollection()->setDefaultShortcut(a,Qt::Key_Right);
    connect (a, &QAction::triggered, game, [this] { game->setMoveDirection(1); });

    // Keys for Singmaster (sm) moves.
    a = mapAction (QStringLiteral("sm_u"), i18n("Move 'Up' face"),
					Qt::Key_U, SM_UP);
    a = mapAction (QStringLiteral("sm_d"), i18n("Move 'Down' face"),
					Qt::Key_D, SM_DOWN);
    a = mapAction (QStringLiteral("sm_l"), i18n("Move 'Left' face"),
					Qt::Key_L, SM_LEFT);
    a = mapAction (QStringLiteral("sm_r"), i18n("Move 'Right' face"),
					Qt::Key_R, SM_RIGHT);
    a = mapAction (QStringLiteral("sm_f"), i18n("Move 'Front' face"),
					Qt::Key_F, SM_FRONT);
    a = mapAction (QStringLiteral("sm_b"), i18n("Move 'Back' face"),
					Qt::Key_B, SM_BACK);
    a = mapAction (QStringLiteral("sm_anti"), i18n("Anti-clockwise move"),
					Qt::Key_Apostrophe, SM_ANTICLOCKWISE);
    a = mapAction (QStringLiteral("sm_plus"), i18n("Singmaster two-slice move"),
					Qt::Key_Plus, SM_2_SLICE);
    a = mapAction (QStringLiteral("sm_minus"), i18n("Singmaster anti-slice move"),
					Qt::Key_Minus, SM_ANTISLICE);
    a = mapAction (QStringLiteral("sm_dot"), i18n("Move an inner slice"),
					Qt::Key_Period, SM_INNER);
    a = mapAction (QStringLiteral("sm_return"), i18n("Complete a Singmaster move"),
					Qt::Key_Return, SM_EXECUTE);
    a = mapAction (QStringLiteral("sm_enter"), i18n("Complete a Singmaster move"),
					Qt::Key_Enter, SM_EXECUTE);
    a = mapAction (QStringLiteral("sm_space"), i18n("Add space to Singmaster moves"),
					Qt::Key_Space, SM_SPACER);

    // IDW - Key for switching the background (temporary) - FIX IT FOR KDE 4.2.
    a = actionCollection()->addAction ( QStringLiteral( "switch_background" ));
    actionCollection()->setDefaultShortcut(a,Qt::Key_K);
    connect (a, &QAction::triggered, game, &Game::switchBackground);
}


QAction * Kubrick::mapAction (const QString & name,
		const QString & text, const Qt::Key key, SingmasterMove mapping)
{
    QAction * a;
    a = actionCollection()->addAction (name);
    a->setText (text);
    actionCollection()->setDefaultShortcut(a,key);
    connect (a, &QAction::triggered, game, [this, mapping] { game->smInput(mapping); });
    return a;
}


void Kubrick::setToggle (const char * actionName, bool onOff)
{
    ((KToggleAction *) ACTION (actionName))->setChecked (onOff);
}


void Kubrick::setAvail (const char * actionName, bool onOff)
{
    ((QAction *) ACTION (actionName))->setEnabled (onOff);
}


void Kubrick::setSingmaster (const QString & smString)
{
    singmasterMoves->setText (smString);
}


void Kubrick::setSingmasterSelection (const int start, const int length)
{
    singmasterMoves->setSelection (start, length);
}


int Kubrick::fillPuzzleList (KSelectAction * s, const PuzzleItem itemList [])
{
    QStringList list;

    for (uint i=0; (strcmp (itemList[i].menuText, "END") != 0); i++) {
	list.append (i18n(itemList[i].menuText));
    }
    s->setItems(list);
    return (list.count() - 1);
}


void Kubrick::fillDemoList (const DemoItem itemList [], QList<QAction *> & list,
				const char * uilist, void(Kubrick::*slot)())
{
    // Generate an action list with one action for each item in the demo list.
    for (uint i = 0; (strcmp (itemList[i].filename, "END") != 0); i++) {
	QAction * t = new QAction (i18n (itemList[i].menuText), this);
    actionCollection()->addAction ( QStringLiteral("%1%2" ).arg(uilist).arg(i), t);
	t->setData (i);		// Save the index of the item inside the action.
	list.append (t);
	connect (t, &QAction::triggered, this, slot);
    }

    // Plug the action list into the Demos menu.
    plugActionList (uilist, list);
}


void Kubrick::saveNewToolbarConfig()
{
    // This destroys our actions lists ...
    KXmlGuiWindow::saveNewToolbarConfig();

    // ... so plug them again
    plugActionList (QStringLiteral("patterns_list"), patternList);
    plugActionList (QStringLiteral("demo_moves_list"), movesList);
}


void Kubrick::easySelected (int index)
{
    statusBarLabel->setText (i18n (easyItems [index].menuText));
    game->changePuzzle (easyItems [index]);
}


void Kubrick::notSoEasySelected (int index)
{
    statusBarLabel->setText (i18n (notSoEasyItems [index].menuText));
    game->changePuzzle (notSoEasyItems [index]);
}


void Kubrick::hardSelected (int index)
{
    statusBarLabel->setText (i18n (hardItems [index].menuText));
    game->changePuzzle (hardItems [index]);
}


void Kubrick::veryHardSelected (int index)
{
    statusBarLabel->setText (i18n (veryHardItems [index].menuText));
    game->changePuzzle (veryHardItems [index]);
}


void Kubrick::patternSelected()
{
    // Retrieve the index of the demo item from the action.
    const QAction * action = static_cast <const QAction *> (sender());
    int index = action->data().toInt();

    if (index > 0) {
	game->loadDemo (patterns[index].filename);
        statusBarLabel->setText (i18n (patterns[index].menuText));
    }
    else {
	KMessageBox::information (this,
		i18n (patternMovesInfo),
		i18n ("Pretty Patterns"));
    }
}


void Kubrick::movesSelected()
{
    // Retrieve the index of the demo item from the action.
    const QAction * action = static_cast <const QAction *> (sender());
    int index = action->data().toInt();

    if (index > 0) {
	game->loadDemo (solvingMoves[index].filename);
        statusBarLabel->setText (i18n (solvingMoves[index].menuText));
    }
    else {
	KMessageBox::information (this,
		i18n (solvingMovesInfo),
		i18n ("Solution Moves"));
    }
}


void Kubrick::describePuzzle (int xDim, int yDim, int zDim, int shMoves)
{
    QString descr;
    if ((xDim == yDim) && (yDim == zDim)) {
	descr = i18n ("%1x%2x%3 cube, %4 shuffling moves",
			xDim, yDim, zDim, shMoves);
    }
    else if ((xDim != 1) && (yDim != 1) && (zDim != 1)) {
	descr = i18n ("%1x%2x%3 brick, %4 shuffling moves",
			xDim, yDim, zDim, shMoves);
    }
    else {
	descr = i18n ("%1x%2x%3 mat, %4 shuffling moves",
			xDim, yDim, zDim, shMoves);
    }
    statusBarLabel->setText (descr);
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


