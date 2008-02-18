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

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H

// Use KDialog and KMessageBox.
#include <KLocale>
#include <KDialog>
#include <KMessageBox>
#include <KGlobalSettings>

#include <QLayout>

#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QFont>

#include "kbkglobal.h"

/**
@author Ian Wadham
*/

class Game;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class GameDialog : public KDialog
{
Q_OBJECT
public:
    GameDialog (bool changePuzzle, int optionTemp [8], QWidget * parent = 0);
    ~GameDialog();

private slots:
    void slotOk();
    void slotHelp();

private:
    Game *		game;
    QWidget *		myParent;
    bool		myChangePuzzle;
    int *		opt;

    QLabel *		dimL;
    QSpinBox *		dimX;		// X dimension.
    QSpinBox *		dimY;		// Y dimension.
    QSpinBox *		dimZ;		// Z dimension.
    QLabel *		shuffleL;
    QSpinBox *		shuffleN;	// Moves per shuffle.
    QCheckBox *		shuffleA;	// Animate the shuffle?
    QCheckBox *		movesA;		// Animate the player's moves?
    QLabel *		speedL;
    QSpinBox *		speedN;		// Speed of moves.
    QLabel *		bevelL;
    QSpinBox *		bevelN;		// % bevel on edges of cubies.
};

#endif
