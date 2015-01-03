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

#ifndef KUBRICK_H
#define KUBRICK_H

#include <KXmlGuiWindow>
#include <KSelectAction>
#include <KShortcutsDialog>

#include "kbkglobal.h"		// Needed for definition of SingmasterMove enum.

class Game;
class GameGLView;
class QAction;
class QSignalMapper;
class QLineEdit;
class QLabel;
class QAction;

/**
 * @short Application Main Window
 * @author Ian Wadham <iandw.au@gmail.com>
 * @version 0.1
 */
class Kubrick : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor.
     */
    Kubrick ();

    /**
     * Default Destructor.
     */
    virtual ~Kubrick ();

    void setToggle      (const char * actionName, bool onOff);
    void setAvail       (const char * actionName, bool onOff);
    void setSingmaster  (const QString & smString);
    void setSingmasterSelection (const int start, const int length);
    void describePuzzle (int xDim, int yDim, int zDim, int shMoves);

    struct PuzzleItem {
	const char * menuText;		// Description of the puzzle, or "END".
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
    bool queryClose();

protected slots:
    void optionsConfigureKeys();

    // Slots for puzzle-selection actions.
    void easySelected      (int index);
    void notSoEasySelected (int index);
    void hardSelected      (int index);
    void veryHardSelected  (int index);

    // Slots for demo-selection actions.
    void patternSelected   ();
    void movesSelected     ();

    void saveNewToolbarConfig();

private:
    Game *       game;			// The game object.
    GameGLView * gameView;		// The main game view.

    QLineEdit *  singmasterMoves;	// A place to display Singmaster moves.
    QLabel *     singmasterLabel;

    static const PuzzleItem easyItems [];
    static const PuzzleItem notSoEasyItems [];
    static const PuzzleItem hardItems [];
    static const PuzzleItem veryHardItems [];

    struct DemoItem {
	const char * filename;		// File containing demo or "END".
	const char * menuText;		// Description of the pattern or moves.
    };
    static const DemoItem patterns [];
    static const char * patternMovesInfo;
    static const DemoItem solvingMoves [];
    static const char * solvingMovesInfo;

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
			const char *uilist, const char *slot);

    QAction * mapAction (QSignalMapper * mapper, const QString & name,
	const QString & text, const Qt::Key key, SingmasterMove mapping);
};

#endif // KUBRICK_H
