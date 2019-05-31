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

#ifndef SCENELABEL_H
#define SCENELABEL_H

#include <QString>
#include <QFont>

class QGLWidget;

class SceneLabel
{
public:
    explicit SceneLabel (const QString & labelText);

    void     setVisible (const bool onOff);
    void     move (const int xPos, const int yPos);
    void     setText (const QString & labelText);
    void     drawLabel (QGLWidget * view);

    int      width () const;
    int      height () const;

private:
    bool     visible;	// Whether to paint the scene-label.
    int	     x;		// x-position in the QGLWidget window.
    int	     y;		// y-position in the QGLWidget window.
    QString  text;	// Text of the scene-label (translated).
    QFont    font;	// Font for the label.
    int      textWidth;	// Width of the text in the given font.
    int      lineHeight;// Height of the font + space between lines.
};

#endif	// SCENELABEL_H
