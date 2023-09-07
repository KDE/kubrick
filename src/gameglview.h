/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef GAMEGLVIEW_H
#define GAMEGLVIEW_H

// Qt includes
#include <QOpenGLWidget>
#include <QMouseEvent>

// Local includes
#include "game.h"
#include "kbkglobal.h"

#include <memory>

class Game;			// Forward declaration of Game class.

class QOpenGLTexture;

/**
 * Demonstrate the features of the Qt OpenGL
 * view and OpenGL itself.
 */
class GameGLView : public QOpenGLWidget
{
public:
    /**
    * Create a new GL view for the game
    **/
    explicit GameGLView (Game * g, QWidget * parent = nullptr);

    /**
    * Dump all OpenGL and GLU extensions to stdout
    **/
    void dumpExtensions     ();

    // IDW - Key K for switching the background (temporary) - FIX IT FOR KDE 4.2
    void changeBackground   ();

    void pushGLMatrix       ();
    void moveGLView         (float xChange, float yChange, float zChange);
    void rotateGLView       (float degrees, float x, float y, float z);
    void popGLMatrix        ();

    void drawACubie         (float size, float centre[], int axis, int angle);
    void finishCubie        ();
    void drawASticker       (float size, int color, bool blinking,
			     int faceNormal[], float faceCentre []);
    void setBlinkIntensity  (float intensity);

    QPoint getMousePosition ();

    void setBevelAmount     (int bevelPerCent);

protected:
    /**
    * Reimplemented to initialize the OpenGL context.
    *
    * This method is called automatically by Qt once.
    */
    void initializeGL() override;

    /**
    * Called by Qt when the size of the GL view changes.
    **/
    void resizeGL(int w, int h) override;

    /**
    * This method actually renders the scene. It is called automatically by Qt
    * when paint events occur. A manual repaint (for example for animations) can
    * be made by calling updateGL which also calls this method.
    *
    * Do not call this method directly!
    **/
    void paintGL() override;

    /**
    * Handle mouse events. In these implementations, game->handleMouseEvent
    * is called with event type, button, X co-ordinate and Y co-ordinate in
    * OpenGL convention (zero at bottom of window).
    **/
    void mousePressEvent   (QMouseEvent* e) override;
    void mouseReleaseEvent (QMouseEvent* e) override;

    /**
    * Check for an OpenGL error. Dump any error to stdout
    * @return true, if an error occurred, otherwise false
    **/
    bool checkGLError();

private:
    BackgroundType backgroundType;
    void     loadBackground (const QString & filepath);
    void     drawPictureBackground();
    void     draw4WayGradientBackground();

    std::unique_ptr<QOpenGLTexture> bgTexture;

    GLfloat  txWidth;
    GLfloat  txHeight;

    GLdouble glAspect;
    GLdouble bgAspect;

    GLfloat  bgRectX;
    GLfloat  bgRectY;
    GLfloat  bgRectZ;

    void turnOnLighting     ();

    Game * game;
    float bevelAmount;		// Fraction of bevelling on a cubie (eg. 0.1).
    QColor bgColor;		// Background color.

    static float colors [7] [3];
    float  blinkIntensity;
};

#endif
