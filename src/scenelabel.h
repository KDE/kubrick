/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SCENELABEL_H
#define SCENELABEL_H

#include <QString>
#include <QFont>

class QOpenGLWidget;

class SceneLabel
{
public:
    explicit SceneLabel (const QString & labelText);

    void     setVisible (const bool onOff);
    void     move (const int xPos, const int yPos);
    void     setText (const QString & labelText);
    void     drawLabel (QOpenGLWidget * view);

    int      width () const;
    int      height () const;

private:
    bool     visible;	// Whether to paint the scene-label.
    int	     x;		// x-position in the QOpenGLWidget window.
    int	     y;		// y-position in the QOpenGLWidget window.
    QString  text;	// Text of the scene-label (translated).
    QFont    font;	// Font for the label.
    int      textWidth;	// Width of the text in the given font.
    int      lineHeight;// Height of the font + space between lines.
};

#endif	// SCENELABEL_H
