/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef GAMEDIALOG_H
#define GAMEDIALOG_H


#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QSpinBox>

#include "kbkglobal.h"

/**
@author Ian Wadham
*/

class Game;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class GameDialog : public QDialog
{
public:
    GameDialog (bool changePuzzle, int optionTemp [8], QWidget * parent = nullptr);
    ~GameDialog() override;

private:
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
