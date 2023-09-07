/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "scenelabel.h"

#include "kbkglobal.h"

#include <QFontMetrics>
#include <QOpenGLWidget>
#include <QPainter>

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
    textWidth   = metrics.boundingRect(text).width();
    lineHeight  = metrics.lineSpacing();
}


void SceneLabel::drawLabel (QOpenGLWidget * view)
{
    if (!visible) {
        return;
    }

    QPainter painter(view);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(x, y, text);
    painter.end();
}


int SceneLabel::width() const
{
    return textWidth;
}


int SceneLabel::height() const
{
    return lineHeight;
}
