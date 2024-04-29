/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "gamedialog.h"
#include <KLocalizedString>
#include <KMessageBox>

#include <QDialogButtonBox>
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>

/******************************************************************************/
/*****************    DIALOG BOX TO SELECT A GAME AND LEVEL   *****************/
/******************************************************************************/

GameDialog::GameDialog (bool changePuzzle, int optionTemp [], QWidget * parent)
                : QDialog (parent)
{
    myParent                 = parent;
    myChangePuzzle           = changePuzzle;
    opt                      = optionTemp;

    QWidget * dad	     = new QWidget (this);
    QVBoxLayout *mainLayout = new QVBoxLayout(dad);
    mainLayout->addWidget(dad);
    setLayout(mainLayout);
    setWindowTitle (i18nc("@title:window", "Rubik's Cube Options"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &GameDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &GameDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QHBoxLayout * cubeDimensions = new QHBoxLayout();
    mainLayout->addLayout (cubeDimensions);

    QHBoxLayout * difficulty = new QHBoxLayout();
    mainLayout->addLayout (difficulty);

    if (changePuzzle) {
	// Lay out spin boxes for the cube dimensions.
	dimL     = new QLabel (i18nc("@lable:spinbox", "Cube dimensions:"));
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

	shuffleL = new QLabel (i18nc("@label:spinbox", "Moves per shuffle (difficulty):"));
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
			"Own…'</nobr> to set the above options.</i>"));
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
    shuffleA = new QCheckBox (i18nc("@option:check", "Watch shuffling in progress?"), dad);
    mainLayout->addWidget (shuffleA);

    movesA   = new QCheckBox (i18nc("@option:check", "Watch your moves in progress?"), dad);
    mainLayout->addWidget (movesA);

    QHBoxLayout * speed = new QHBoxLayout();
    mainLayout->addLayout (speed);

    speedL   = new QLabel (i18nc("@label:spinbox", "Speed of moves:"));
    speedN   = new QSpinBox ();
    speedN->setRange (1, 15);
    speed->addWidget (speedL);
    speed->addWidget (speedN);

    QHBoxLayout * bevel = new QHBoxLayout();
    mainLayout->addLayout (bevel);

    // xgettext: no-c-format
    bevelL   = new QLabel (i18nc("@label:spinbox", "% of bevel on edges of cubies:"));
    bevelN   = new QSpinBox ();
    bevelN->setRange (4, 30);
    bevelN->setSingleStep (2);
    bevel->addWidget (bevelL);
    bevel->addWidget (bevelN);

    mainLayout->addWidget(buttonBox);

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

    connect(okButton, &QPushButton::clicked, this, &GameDialog::slotOk);
    connect(buttonBox->button(QDialogButtonBox::Help), &QPushButton::clicked, this, &GameDialog::slotHelp);
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


