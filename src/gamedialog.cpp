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

#include "gamedialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

/******************************************************************************/
/*****************    DIALOG BOX TO SELECT A GAME AND LEVEL   *****************/
/******************************************************************************/

GameDialog::GameDialog (bool changePuzzle, int optionTemp [], QWidget * parent)
                : KDialog (parent)
{
    myParent                 = parent;
    myChangePuzzle           = changePuzzle;
    opt                      = optionTemp;

    int margin		     = marginHint();
    int spacing		     = spacingHint();

    QWidget * dad	     = new QWidget (this);
    setMainWidget (dad);
    setCaption (i18n ("Rubik's Cube Options"));
    setButtons (KDialog::Ok | KDialog::Cancel | KDialog::Help);
    setDefaultButton (KDialog::Ok);

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QHBoxLayout * cubeDimensions = new QHBoxLayout();
    mainLayout->addLayout (cubeDimensions);
    cubeDimensions->setSpacing (spacing);

    QHBoxLayout * difficulty = new QHBoxLayout();
    mainLayout->addLayout (difficulty);
    difficulty->setSpacing (spacing);

    if (changePuzzle) {
	// Lay out spin boxes for the cube dimensions.
	dimL     = new QLabel (i18n("Cube dimensions:"));
	cubeDimensions->addWidget (dimL);

	dimX     = new QSpinBox ();
	dimY     = new QSpinBox ();
	dimZ     = new QSpinBox ();
	dimX->setRange (1, 6);
	dimY->setRange (1, 6);
	dimZ->setRange (1, 6);
	cubeDimensions->addWidget (dimX);
	cubeDimensions->addWidget (dimY);
	cubeDimensions->addWidget (dimZ);

	shuffleL = new QLabel (i18n("Moves per shuffle (difficulty):"));
	shuffleN = new QSpinBox ();
	shuffleN->setRange (0, 50);
	difficulty->addWidget (shuffleL);
	difficulty->addWidget (shuffleN);
    }
    else {
	dimL     = new QLabel (i18n("Cube dimensions: %1x%2x%3",
			optionTemp [optXDim],
			optionTemp [optYDim],
			optionTemp [optZDim]));
	cubeDimensions->addWidget (dimL);

	shuffleL = new QLabel (i18n("Moves per shuffle (difficulty): %1",
			optionTemp [optShuffleMoves]));
	difficulty->addWidget (shuffleL);

	QHBoxLayout * msg = new QHBoxLayout();
	mainLayout->addLayout (msg);
	QLabel * msgL = new QLabel (i18n(
			"<i>Please use <nobr>'Choose Puzzle Type->Make Your "
			"Own...'</nobr> to set the above options.</i>"));
        msgL->setWordWrap(true);
        msg->addWidget (msgL);
    }

    QFont f  = dimL->font();
    f.setBold (true);
    dimL->setFont (f);
    shuffleL->setFont (f);

    // Add a horizontal line as a separator.
    QFrame * separator = new QFrame (dad);
    separator->setFrameShape  (QFrame::HLine);
    separator->setFrameShadow (QFrame::Sunken);
    mainLayout->addWidget (separator);

    // Lay out the remaining options.
    shuffleA = new QCheckBox (i18n("Watch shuffling in progress?"), dad);
    mainLayout->addWidget (shuffleA);

    movesA   = new QCheckBox (i18n("Watch your moves in progress?"), dad);
    mainLayout->addWidget (movesA);

    QHBoxLayout * speed = new QHBoxLayout();
    mainLayout->addLayout (speed);
    speed->setSpacing (spacing);

    speedL   = new QLabel (i18n("Speed of moves:"));
    speedN   = new QSpinBox ();
    speedN->setRange (1, 15);
    speed->addWidget (speedL);
    speed->addWidget (speedN);

    QHBoxLayout * bevel = new QHBoxLayout();
    mainLayout->addLayout (bevel);
    bevel->setSpacing (spacing);

    // xgettext: no-c-format
    bevelL   = new QLabel (i18n("% of bevel on edges of cubies:"));
    bevelN   = new QSpinBox ();
    bevelN->setRange (4, 30);
    bevelN->setSingleStep (2);
    bevel->addWidget (bevelL);
    bevel->addWidget (bevelN);

    // Set the option-widgets to the current values of the options.
    if (changePuzzle) {
	dimX->setValue (optionTemp [optXDim]);
	dimY->setValue (optionTemp [optYDim]);
	dimZ->setValue (optionTemp [optZDim]);

	shuffleN->setValue   (optionTemp [optShuffleMoves]);
    }
    shuffleA->setChecked ((bool) optionTemp [optViewShuffle]);
    movesA->setChecked   ((bool) optionTemp [optViewMoves]);
    speedN->setValue     (optionTemp [optMoveSpeed]);

    bevelN->setValue     (optionTemp [optBevel]);

    connect (this, SIGNAL (okClicked()),     this, SLOT (slotOk()));
    connect (this, SIGNAL (helpClicked()),   this, SLOT (slotHelp()));
}


GameDialog::~GameDialog()
{
}


void GameDialog::slotOk ()
{
    // Pass back the latest option values.
    if (myChangePuzzle) {
	// We are making a new cube.
	opt [optXDim]         = dimX->value();
	opt [optYDim]         = dimY->value();
	opt [optZDim]         = dimZ->value();

	opt [optShuffleMoves] = shuffleN->value();
    }

    opt [optViewShuffle]  = (int) shuffleA->isChecked();
    opt [optViewMoves]    = (int) movesA->isChecked();
    opt [optMoveSpeed]    = speedN->value();

    opt [optBevel]        = bevelN->value();
}


void GameDialog::slotHelp ()
{
    // Help for "Rubik's Cube Options" dialog box.
    QString s = i18n(
	"You can choose any size of cube (or brick) up to 6x6x6, but only "
	"one side can have dimension 1 (otherwise the puzzle becomes trivial). "
	" The easiest puzzle is 2x2x1 and 3x3x1 is a good warmup for the "
	"original Rubik's Cube, which is 3x3x3.  "
	"Simple puzzles have 2 to 5 shuffling moves, a difficult 3x3x3 puzzle "
	"has 10 to 20 --- or you can choose zero shuffling then shuffle the "
	"cube yourself, maybe for a friend to solve.\n"
	"The other options determine whether you can watch the shuffling "
	"and/or your own moves and how fast the animation goes.  The bevel "
	"option affects the appearance of the small cubes.  Try setting "
	"it to 30 and you'll see what we mean.");

    KMessageBox::information (myParent, s, i18n("HELP: Rubik's Cube Options"));
}

#include "gamedialog.moc"
