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

#include "scenelabel.h"

#include "kbkglobal.h"

#include <QFontMetrics>
#include <QGLWidget>

SceneLabel::SceneLabel (const QString & labelText)
{
    font.setPointSize (12);
    font.setWeight (QFont::Bold);

    visible     = true;
    x           = 0;
    y           = 0;
    setText (labelText);
}


void SceneLabel::setVisible (const bool onOff)
{
    visible     = onOff;
}


void SceneLabel::move (const int posX, const int posY)
{
    x           = posX;
    y           = posY;
}


void SceneLabel::setText (const QString & labelText)
{
    QFontMetrics metrics (font);

    text        = labelText;
    textWidth   = metrics.width(text);
    lineHeight  = metrics.lineSpacing();
}


void SceneLabel::drawLabel (QGLWidget * view)
{
    if (visible) {
	glColor3f (1.0, 1.0, 1.0);
	view->renderText (x, y, text, font);
    }
}


int SceneLabel::width() const
{
    return textWidth;
}


int SceneLabel::height() const
{
    return lineHeight;
}
