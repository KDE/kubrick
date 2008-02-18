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

#ifndef _KUBRICK_H_
#define _KUBRICK_H_

#include <KXmlGuiWindow>
#include <KStandardAction>
#include <KSelectAction>
#include <KShortcutsDialog>

class Game;
class GameGLView;

/**
 * @short Application Main Window
 * @author Ian Wadham <ianw@netspace.net.au>
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

    void setToggle   (const char * actionName, bool onOff);
    void setAvail    (const char * actionName, bool onOff);

    struct PuzzleItem {
	char * menuText;		// Description of the puzzle, or "END".
	int    x;			// X dimension.
	int    y;			// Y dimension.
	int    z;			// Z dimension.
	int    shuffleMoves;		// Number of shuffling moves.
	bool   viewShuffle;		// Whether to animate shuffling moves.
	bool   viewMoves;		// Whether to animate the user's moves.
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
    void patternSelected      (int index);
    void movesSelected        (int index);

private:
    Game *       game;			// The game object.
    GameGLView * gameView;		// The main game view.

    static const PuzzleItem easyItems [];
    static const PuzzleItem notSoEasyItems [];
    static const PuzzleItem hardItems [];
    static const PuzzleItem veryHardItems [];

    struct DemoItem {
	const char * filename;		// File containing demo or "END".
	const char * menuText;		// Description of the pattern or moves.
    };
    static const DemoItem patterns [];
    static const QString patternMovesInfo;
    static const DemoItem solvingMoves [];
    static const QString solvingMovesInfo;

    // Puzzle-selection actions.
    KSelectAction * easyList;
    KSelectAction * notSoEasyList;
    KSelectAction * hardList;
    KSelectAction * veryHardList;

    // Demo-selection actions.
    KSelectAction * patternList;
    KSelectAction * movesList;
    int maxPatternsIndex;
    int maxMovesIndex;

    int fillPuzzleList (KSelectAction * s, const PuzzleItem itemList []);
    int fillDemoList   (KSelectAction * s, const DemoItem   itemList []);
};

#endif // _KUBRICK_H_
