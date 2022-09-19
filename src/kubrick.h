/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KUBRICK_H
#define KUBRICK_H

#include <KXmlGuiWindow>
#include <KSelectAction>
#include <KShortcutsDialog>
#include <KLazyLocalizedString>

#include "kbkglobal.h"		// Needed for definition of SingmasterMove enum.

class Game;
class GameGLView;
class QLabel;
class QAction;
class QLineEdit;
class QAction;

/**
 * @short Application Main Window
 * @author Ian Wadham <iandw.au@gmail.com>
 * @version 0.1
 */
class Kubrick : public KXmlGuiWindow
{
public:
    /**
     * Default Constructor.
     */
    Kubrick ();

    /**
     * Default Destructor.
     */
    ~Kubrick () override;

    void setToggle      (const char * actionName, bool onOff);
    void setAvail       (const char * actionName, bool onOff);
    void setSingmaster  (const QString & smString);
    void setSingmasterSelection (const int start, const int length);
    void describePuzzle (int xDim, int yDim, int zDim, int shMoves);

    struct PuzzleItem {
    const KLazyLocalizedString menuText;		// Description of the puzzle, or "END".
	const int    x;			// X dimension.
	const int    y;			// Y dimension.
	const int    z;			// Z dimension.
	const int    shuffleMoves;	// Number of shuffling moves.
	const bool   viewShuffle;	// Whether to animate shuffling moves.
	const bool   viewMoves;		// Whether to animate the user's moves.
    };

protected:
    /**
     * Initialise the GUI elements (menus, keystroke actions, etc.).
     */
    void initGUI();

    /**
     * Exit the program: invoked during Quit action or window-close by X-widget.
     * It saves the current cube and its state unconditionally, except when
     * there is a demo in progress and the player's cube and state have already
     * been saved.  This is as with a real Rubik Cube, which stays how it is
     * when you stop playing with it.
     */
    bool queryClose() override;

protected:
    void optionsConfigureKeys();

    // Slots for puzzle-selection actions.
    void easySelected      (int index);
    void notSoEasySelected (int index);
    void hardSelected      (int index);
    void veryHardSelected  (int index);

    // Slots for demo-selection actions.
    void patternSelected   ();
    void movesSelected     ();

    void saveNewToolbarConfig() override;

private:
    Game *       game;			// The game object.
    GameGLView * gameView;		// The main game view.

    QLineEdit *  singmasterMoves;	// A place to display Singmaster moves.
    QLabel *     singmasterLabel;
    QLabel *     statusBarLabel;

    static const PuzzleItem easyItems [];
    static const PuzzleItem notSoEasyItems [];
    static const PuzzleItem hardItems [];
    static const PuzzleItem veryHardItems [];

    struct DemoItem {
	const char * filename;		// File containing demo or "END".
    const KLazyLocalizedString menuText;		// Description of the pattern or moves.
    };
    static const DemoItem patterns [];
    static const KLazyLocalizedString patternMovesInfo;
    static const DemoItem solvingMoves [];
    static const KLazyLocalizedString solvingMovesInfo;

    // Puzzle-selection actions.
    KSelectAction * easyList;
    KSelectAction * notSoEasyList;
    KSelectAction * hardList;
    KSelectAction * veryHardList;

    // Demo-selection actions.
    QList<QAction*> patternList;
    QList<QAction*> movesList;

    int fillPuzzleList (KSelectAction * s, const PuzzleItem itemList []);
    void fillDemoList  (const DemoItem itemList [], QList<QAction*> &list,
			const char *uilist, void(Kubrick::*slot)());

    QAction * mapAction (const QString & name,
	const QString & text, const Qt::Key key, SingmasterMove mapping);
};

#endif // KUBRICK_H
