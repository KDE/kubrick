/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Local includes
#include "gameglview.h"

// Qt includes
#include <QOpenGLTexture>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPoint>
#include <QStandardPaths>
#include <QStringList>
#include <QSvgRenderer>

#include "kubrick_debug.h"
// C++ includes
#include <iostream>

GameGLView::GameGLView(Game * g, QWidget * parent)
            : QOpenGLWidget(parent),
              bgColor (QColor (0, 0, 35))
{
    // Save a pointer to the Game object that controls everything.
    game = g;

    this->setMinimumSize (450, 300);

    // Set the amount of bevelling on a cubie's edge to its default value.
    bevelAmount = 0.125;

    // Set colors to full intensity (no blinking).
    blinkIntensity = 1.0;

    // Note: Do not use OpenGL functions here.
    //       Initialize OpenGL in initializeGL() instead!

    const GLubyte * v = glGetString(GL_VERSION);
    printf ("GL Version %s\n", v);
}


void GameGLView::initializeGL()
{
    // Look for themes in files "---/share/apps/kubrick/themes/*.desktop".
    // IDW - This is temporary code for KDE 4.1. Do themes properly in KDE 4.2.
    QStringList themeFilepaths;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("themes"), QStandardPaths::LocateDirectory);
    for (const QString& dir : dirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.svgz"));  // Find files.
        for (const QString& file : fileNames) {
            themeFilepaths.append(dir + QLatin1Char('/') + file);
        }
    }
    if (! themeFilepaths.isEmpty()) {
	backgroundType = PICTURE;	// Use a picture for the background.
	loadBackground (themeFilepaths.first());
    }
    else {
	backgroundType = NO_LOAD;	// Use a 4-way color gradient.
    }

    // Make glClear() clear to a deep-blue background.
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());

    // Disable dithering which is usually not needed.
    glDisable(GL_DITHER);

    // Set up fixed lighting.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    turnOnLighting();
}


// IDW - Key K for switching the background (temporary) - FIX IT FOR KDE 4.2.
void GameGLView::changeBackground()
{
    if (backgroundType != NO_LOAD) {
	backgroundType = (backgroundType == PICTURE) ? GRADIENT : PICTURE;
    }
}


void GameGLView::loadBackground (const QString & filepath)
{
    // NOTE: Size 1024 gave a significantly sharper picture, compared to 512.
    int bgTextureSize = 1024;		// Size of background texture.
    QImage tex (bgTextureSize, bgTextureSize, QImage::Format_ARGB32);
    tex.fill (bgColor.rgba());

    QString bg (QStringLiteral("background"));
    GLdouble bgWidth  = bgTextureSize;
    GLdouble bgHeight = bgTextureSize;
    bgAspect = 1.0;

    QSvgRenderer svg;
    svg.load (filepath);

    if (svg.isValid()) {
	QRectF r = svg.boundsOnElement (bg);

	bgAspect = r.width() / r.height();
	bool landscape = (bgAspect >= 1.0);

	bgWidth  = landscape ? bgTextureSize : bgTextureSize * bgAspect;
	bgHeight = landscape ? bgTextureSize / bgAspect : bgTextureSize;

	// Render the drawing at the bottom left of the texture's QImage.
	QRectF bounds (0, bgTextureSize - bgHeight, bgWidth, bgHeight);
	QPainter p;
	p.begin (&tex);
	svg.render (&p, bg, bounds);
	p.end();
    }
    else {
	backgroundType = NO_LOAD;	// Use a 4-way color gradient.
    }

    bgTexture.reset(new QOpenGLTexture(tex.mirrored()));
    txWidth  = bgWidth  / (GLfloat) bgTextureSize;
    txHeight = bgHeight / (GLfloat) bgTextureSize;
}


void GameGLView::resizeGL(int w, int h)
{
    // Make use of the whole view.
    glViewport (0, 0, w, h);
    glAspect = (GLdouble) w / (GLdouble) h;

    // Set the perspective projection.
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    gluPerspective (viewAngle, glAspect, minZ, maxZ);
    glMatrixMode (GL_MODELVIEW);

    // Calculate the depth and size of the background rectangle.
    bgRectZ = 2.0 * cubeCentreZ;
    bgRectY = -bgRectZ * tan (3.14159 * viewAngle / 360.0);
    bgRectX = glAspect * bgRectY;

    if (backgroundType == PICTURE) {
	// Make the aspect ratio the same as for the original SVG picture.
	if (glAspect > bgAspect) {
	    // Fit background to full width: OpenGL will trim top and bottom.
	    bgRectY = bgRectX / bgAspect;	// Stretch the height.
	}
	else {
	    // Fit background to full height: OpenGL will trim left and right.
	    bgRectX = bgRectY * bgAspect;	// Stretch the width.
	}
    }

    // Re-position the view-labels.
    game->setSceneLabels();
}


float GameGLView::colors [7] [3] = {
	{0.2, 0.2, 0.2},	// Dark grey.
	{0.7, 0.0, 0.0},	// Red.
	{1.0, 0.5, 0.1},	// Orange.
	{0.0, 0.0, 0.8},	// Blue.
	{0.0, 0.5, 0.0},	// Green.
	{1.0, 1.0, 0.0},	// Yellow.
	{0.9, 0.9, 0.8}		// Off-white.
	};


void GameGLView::paintGL()
{
    // Clear the view.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the modelview matrix.  By doing so we also reset the "camera" that
    // is the eye position, so that it is now at (0,0,0), which is also the
    // center of the screen.  We are looking down the negative Z axis, towards
    // (0,0,-1), with "up" being at (0,1,0).
    glLoadIdentity();
    glEnable (GL_DEPTH_TEST);

    if (checkGLError()) {
	std::cerr << "OpenGL error detected before drawScene()" << std::endl;
    }

    // Draw the background.
    glDisable (GL_LIGHTING);
    switch (backgroundType) {
    case PICTURE:
	drawPictureBackground();
	break;
    case GRADIENT:
    case NO_LOAD:
	draw4WayGradientBackground();
	break;
    }
    glEnable (GL_LIGHTING);

    // Draw the cube(s).
    game->drawScene ();

    if (checkGLError()) {
	std::cerr << "OpenGL error detected after drawScene()" << std::endl;
    }
}


void GameGLView::drawPictureBackground()
{
    glEnable (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, bgTexture->textureId());

    // Draw the background picture behind the cubes, filling the view.
    glBegin (GL_QUADS);
	glTexCoord2f (0.0,     0.0);
	glVertex3f   (-bgRectX, -bgRectY, bgRectZ);

	glTexCoord2f (txWidth, 0.0);
	glVertex3f   ( bgRectX, -bgRectY, bgRectZ);

	glTexCoord2f (txWidth, txHeight);
	glVertex3f   ( bgRectX,  bgRectY, bgRectZ);

	glTexCoord2f (0.0,     txHeight);
	glVertex3f   (-bgRectX,  bgRectY, bgRectZ);
    glEnd();

    glDisable (GL_TEXTURE_2D);
}


void GameGLView::draw4WayGradientBackground()
{
    glShadeModel (GL_SMOOTH);

    // Draw the 4-way color-gradient behind the cubes, filling the view.
    glBegin(GL_QUADS);
	glColor3f (0.0, 0.0, 0.5);
	glVertex3f   (-bgRectX, -bgRectY, bgRectZ);

	glColor3f (0.8, 0.3, 0.6);
	glVertex3f   ( bgRectX, -bgRectY, bgRectZ);

	glColor3f (0.5, 0.8, 1.0);
	glVertex3f   ( bgRectX,  bgRectY, bgRectZ);

	glColor3f (0.2, 0.2, 0.9);
	glVertex3f   (-bgRectX,  bgRectY, bgRectZ);
    glEnd();

    glShadeModel (GL_FLAT);
}


void GameGLView::dumpExtensions()
{
    // OpenGL Extension detection.
    QString s = (const char*)glGetString(GL_EXTENSIONS);
    s += QLatin1Char(' ');
    s += (const char*)gluGetString(GLU_EXTENSIONS);
    QStringList extensions = s.split (' ', Qt::SkipEmptyParts);
    for (int i = 0; i < extensions.count(); i++)
    {
	std::cout << extensions[i].toLatin1().data() << std::endl;
    }
}

static float ambient[] = {0.0, 0.0, 0.0, 1.0};
static float diffuse[] = {1.0, 1.0, 1.0, 1.0};

// Directional light, at infinity (parameter 4, w=0.0), shining
// towards -Z.  No attenuation.  Diffuse, no spotlight effect.
static float position0[] = {0.0, 0.0, 1.0, 0.0};

// With lights off, 100% ambient gives a flat, dull, fairly well-lit picture.
// With lights on, it makes little difference to specular effect, but makes
// the strong colors of stickers look a bit washed out.  So choose 60%.
static float lmodel_ambient[] = {0.6, 0.6, 0.6, 1.0};

// Controls SPREAD of specular reflections (128.0 = small highlight).
static float material_shininess = 60.0;

// Controls intensity and color of specular reflections.
static float material_specular[] = {0.5, 0.6, 0.5, 1.0};

// Trying to light the insides of the cubies is a waste of time.
static float lmodel_twoside[] = {GL_FALSE};

void GameGLView::turnOnLighting ()
{
    // There is just one light.
    glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT0, GL_POSITION, position0);

    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv (GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

    // This gives a rolling-sheen effect to the specularity.
    // Not all stickers on a face light up at once.
    glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT0);

    glEnable (GL_DEPTH_TEST);
    glEnable (GL_NORMALIZE);

    // Do not render the backs (interiors) of cubie and sticker faces. The
    // "front" faces are those for which the order of drawing of the vertices
    // is anti-clockwise, as seen from the outside looking towards the centre.
    glEnable (GL_CULL_FACE);

    glShadeModel (GL_FLAT);
    glMaterialf  (GL_FRONT, GL_SHININESS, material_shininess);
    glMaterialfv (GL_FRONT, GL_SPECULAR,  material_specular);

    // This tracks color CHANGES and updates the corresponding glMaterial*.
    glEnable (GL_COLOR_MATERIAL);
    glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}


QPoint GameGLView::getMousePosition ()
{
    QPoint p;
    p = mapFromGlobal (QCursor::pos());	// Convert to window co-ordinates.

    // Convert Y co-ordinate to OpenGL convention (zero at bottom of window).
    p.setY (height() - p.y());
    return p;
}


void GameGLView::pushGLMatrix ()
{
    glPushMatrix();
}


void GameGLView::moveGLView (float xChange, float yChange, float zChange)
{
    glTranslatef (xChange, yChange, zChange);
}


void GameGLView::rotateGLView (float degrees, float x, float y, float z)
{
    glRotatef (degrees, x, y, z);
}


void GameGLView::popGLMatrix ()
{
    glPopMatrix();
}


void GameGLView::setBevelAmount (int bevelPerCent)
{
    bevelAmount = ((float) bevelPerCent)/100.0;
}


void GameGLView::drawACubie (float size, float centre[], int axis, int angle)
{
    float lenA = 0.5 * size;		// Half of a cubie's outside dimension.
    float bevel = bevelAmount * lenA;	// The size of the bevel at the edges.
    float lenB = lenA - bevel;		// Half of a face's inside dimension.

    float p[nAxes];			// The point to be drawn currently.
    float normal[nAxes];		// The current normal vector (used in lighting).
				// Needs to be of length 1 for best results.
    float r2 = 0.7071067;	// (1 / sqrt (2.0)), used for bevel normals.
    float r3 = 0.5773502;	// (1 / sqrt (3.0)), used for corner normals.

    // If the cubie is moving, rotate it around the required axis of the cube.
    if (angle != 0) {
	GLfloat v [nAxes] = {0.0, 0.0, 0.0};
	v [axis] = 1.0;
	glPushMatrix();
	glRotatef ((GLfloat) (-angle), v[0], v[1], v[2]);
    }

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glPushMatrix();

    glColor3fv (colors [0]);	// The base color of a cubie, without stickers.

    // Bring the centre to the origin during drawing.
    glTranslatef ((GLfloat) centre[X],
		  (GLfloat) centre[Y],
		  (GLfloat) centre[Z]);

    // Draw three pairs of faces of the cubie and twelve bevelled edges.
    glBegin(GL_QUADS);
    LOOP (axis, 3) {

	// The coordinate on the axis thru the face does not change, but we must
	// work out what the other two coordinates are, in cyclical order: i.e.
	//         X-axis (0) --> Y,Z (coordinates 1 and 2 change),
	//         Y-axis (1) --> Z,X (coordinates 2 and 0 change),
	//         Z-axis (2) --> X,Y (coordinates 0 and 1 change). 
	//
        int coord1 = 0;
        int coord2 = 0;

	// Draw two opposite faces of the cubie and two bevelled edges each.
	LOOP (face, 2) {

	    // Draw one face of the cubie.
	    // For (i,j) = (0,0), (0,1), (1,0) and (1,1), we will get values
	    //     (i,k) = (0,0), (0,1), (1,1) and (1,0) and will be drawing
	    // vertices going around a square face of the cubie.

	    normal[coord1] = 0.0;
	    normal[coord2] = 0.0;
	    normal[axis] = (2*face -1) * 1.0;	// -1.0 or +1.0.
	    glNormal3fv (normal);		// Used by lighting.

	    p[axis] = (2*face-1) * lenA;	// Value -lenA or +lenA.
	    // The axes are chosen so that the vertices go in "GL front face"
	    // order (ie. anti-clockwise), as viewed from outside the cubie.
	    if (p[axis] > 0.0) {
		coord1 = (axis + 2) % 3;
		coord2 = (axis + 1) % 3;
	    }
	    else {
		coord1 = (axis + 1) % 3;
		coord2 = (axis + 2) % 3;
	    }

	    LOOP (i, 2) {
		p[coord1] = (2*i-1) * lenB;	// Value -lenB or +lenB.
		LOOP (j, 2) {
		    int k = (i + j) % 2;	// Takes values 0 1 1 0.
		    p[coord2] = (2*k-1) * lenB;	// Value -lenB or +lenB.
		    glVertex3fv (p);		// Draw one of 4 vertices.
		}
	    }

	    // Draw two of this face's bevelled edges.
	    // Eventually, for 6 faces, all 12 edges will have been drawn.
	    coord1 = (axis + 1) % 3;
	    coord2 = (axis + 2) % 3;
	    normal[axis] = (2*face -1) * r2;
	    LOOP (edge, 2) {
		normal[coord1] = (2*edge -1) * r2;
		normal[coord2] = 0.0;
		glNormal3fv (normal);		// Used by lighting.

		LOOP (i, 2) {
		    p[axis]   = (2*face-1) * (lenA - ((edge + i)%2) * bevel);
		    p[coord1] = (2*edge-1) * lenB + (edge - i) * bevel;
		    LOOP (j, 2) {
			int k = (i + j) % 2;	// Takes values 0 1 1 0.
			// p[coord2] = -lenB, +lenB, +lenB, -lenB on the + face
			//       and = +lenB, -lenB, -lenB, +lenB on the - face,
			// thus ensuring anti-clockwise drawing of all 4 bevels
			// as you view each face from the outside looking in.
			p[coord2] = (2*face -1) * (2*k-1) * lenB;
			glVertex3fv (p);	// Draw 1 of 4 vertices.
		    }
		}
	    } // END (edge, 2)

	} // END (face, 2)

    } // END (axis, 3)

    glEnd();

    // Draw the 8 chiselled corners on the back and front faces of the cubie.
    glBegin(GL_TRIANGLES);
    LOOP (face, 2) {
	float z = ((float) 2*face-1);		// Z direction: +r3 or -r3.
	LOOP (i, 2) {
	    float x = ((float) 2*i-1);		// X direction: +r3 or -r3.
	    LOOP (j, 2) {
		float y = ((float) 2*j-1);	// Y direction: +r3 or -r3.

		glNormal3f (x*r3, y*r3, z*r3);	// Used by lighting.

		// Draw the triangular facet at this corner of the face.
		// The vertices must go in "GL front face" order (ie. anti-
		// clockwise), as viewed from outside the cubie.

		if ((x * y * z) > 0.0) {
		    // If 0 or 2 negatives, go round the vertices one way.
		    glVertex3f (x*lenA, y*lenB, z*lenB);	// A
		    glVertex3f (x*lenB, y*lenA, z*lenB);	// B
		    glVertex3f (x*lenB, y*lenB, z*lenA);	// C
		}
		else {
		    // If 1 or 3 negatives, go round the vertices the other way.
		    glVertex3f (x*lenA, y*lenB, z*lenB);	// A
		    glVertex3f (x*lenB, y*lenB, z*lenA);	// C
		    glVertex3f (x*lenB, y*lenA, z*lenB);	// B
		}
	    }
	}
    } // END (face, 2)

    glEnd();

    glPopAttrib();
    glPopMatrix();
    checkGLError();
}


void GameGLView::finishCubie ()
{
    // If the cubie just drawn is moving, restore the OpenGL co-ordinate axes.
    glPopMatrix();
}


void GameGLView::drawASticker (float size, int color, bool blinking,
			       int faceNormal[], float faceCentre [])
{
    float lenA = 0.5   * size;		// Half of a cubie's outside dimension.
    float bevel = bevelAmount * lenA;	// The size of the bevel at the edges.
    float lenB = lenA - bevel;		// Half the sticker's dimension.

    float p[nAxes];			// The point to be drawn currently.
    float normal[nAxes];		// The normal vector (used in lighting).

    int axis1 = 0, axis2 = 1;		// Two axes in the plane of the sticker.

    float mColor [3];

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glPushMatrix();

    // Set the color of this sticker.
    if (blinking) {
	LOOP (i, 3) {
	    mColor [i] = blinkIntensity * colors [color] [i];
	}
	glColor3fv (mColor);		// Dimmed color.
    }
    else {
	glColor3fv (colors [color]);	// Normal color.
    }

    // Bring the centre of the sticker to the origin during drawing.
    glTranslatef ((GLfloat) faceCentre[X],
                  (GLfloat) faceCentre[Y],
                  (GLfloat) faceCentre[Z]);

    LOOP (axis, 3) {
	normal [axis] = 0.0;
	if (faceNormal [axis] != 0) {
	    // One component of the faceNormal vector is 1: the others are zero.
	    normal [axis] = (float) faceNormal [axis];

	    // The sticker is offset from the cubie's face, along the normal.
	    p [axis] = 0.01 * size * normal [axis];

	    // The coordinate on the normal does not change, but we must work
	    // out what the other two coordinates are, in cyclical order: i.e.
	    //         X-axis (0) --> Y,Z (coordinates 1 and 2 change),
	    //         Y-axis (1) --> Z,X (coordinates 2 and 0 change),
	    //         Z-axis (2) --> X,Y (coordinates 0 and 1 change). 
	    //
	    // Ensure that the outside face of the sticker is drawn in
	    // anti-clockwise sequence, so GL_CULL_FACE culls the inside one.

	    if (faceNormal [axis] > 0) {
		axis1 = (axis + 1) % 3;
		axis2 = (axis + 2) % 3;
	    }
	    else {
		axis2 = (axis + 1) % 3;
		axis1 = (axis + 2) % 3;
	    }
	}
    }

    glBegin(GL_QUADS);
    glNormal3fv (normal);	// Used by lighting.

    p [axis1] = + lenB;		// Set the first vertex of the sticker.
    p [axis2] = + lenB;
    glVertex3fv (p);		// Draw the first vertex,  eg. (+X,+Y) quadrant.
    p [axis1] = - lenB;
    glVertex3fv (p);		// Draw the second vertex, eg. (+X,-Y) quadrant.
    p [axis2] = - lenB;
    glVertex3fv (p);		// Draw the third vertex,  eg. (-X,-Y) quadrant.
    p [axis1] = + lenB;
    glVertex3fv (p);		// Draw the fourth vertex, eg. (+X,-Y) quadrant.

    glEnd();

    glPopAttrib();
    glPopMatrix();
    checkGLError();
}


void GameGLView::setBlinkIntensity (float intensity)
{
    blinkIntensity = intensity;
}


bool GameGLView::checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
	// std::cerr << "An OpenGL error occurred" << std::endl;
	return true;
    }
    return false;
}


void GameGLView::mousePressEvent(QMouseEvent* e)
{
    // ensure our context outside of paint event when querying scene data
    makeCurrent();
    // Convert Y co-ordinate to OpenGL convention (zero at bottom of window).
    game->handleMouseEvent (ButtonDown, e->button(),
				e->pos().x(), height() - e->pos().y());
}


void GameGLView::mouseReleaseEvent(QMouseEvent* e)
{
    // ensure our context outside of paint event when querying scene data
    makeCurrent();
    // Convert Y co-ordinate to OpenGL convention (zero at bottom of window).
    game->handleMouseEvent (ButtonUp, e->button(),
				e->pos().x(), height() - e->pos().y());
}

// End gameglview.cpp


