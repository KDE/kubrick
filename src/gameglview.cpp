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

// Qt includes
#include <QStringList>
#include <QPoint>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QtOpenGL>

// C++ includes
#include <iostream>

// Local includes
#include "gameglview.h"

GameGLView::GameGLView(Game * g, QWidget * parent)
            : QGLWidget(QGLFormat (QGL::AlphaChannel), parent),
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
    // Make glClear() clear to a deep-blue background.
    qglClearColor (bgColor);	

    // Disable dithering which is usually not needed
    glDisable(GL_DITHER);

    // Set up fixed lighting.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    turnOnLighting();
}


void GameGLView::resizeGL(int w, int h)
{
    // Make use of the whole view.
    glViewport(0, 0, w, h);

    // Setup a 3D projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(viewAngle, ((GLdouble)w)/((GLdouble)h), minZ, maxZ);
    glMatrixMode(GL_MODELVIEW);

    // Re-position the view-labels.
    game->setSceneLabels ();
}


float GameGLView::colors [7] [3] = {
	{0.2, 0.2, 0.2},	// Dark grey.
	{1.0, 0.5, 0.1},	// Orange.
	{0.7, 0.0, 0.0},	// Red.
	{1.0, 1.0, 0.0},	// Yellow.
	{0.0, 0.5, 0.0},	// Green.
	{0.0, 0.0, 0.8},	// Blue.
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

    if (checkGLError()) {
	std::cerr << "OpenGL error detected before drawScene()" << std::endl;
    }

    game->drawScene ();

    if (checkGLError()) {
	std::cerr << "OpenGL error detected after drawScene()" << std::endl;
    }
}


void GameGLView::dumpExtensions()
{
    // OpenGL Extension detection.
    QString s = (const char*)glGetString(GL_EXTENSIONS);
    s += " ";
    s += (const char*)gluGetString(GLU_EXTENSIONS);
    QStringList extensions = s.split (" ", QString::SkipEmptyParts);
    for (int i = 0; i < extensions.count(); i++)
    {
	std::cout << extensions[i].toLatin1().data() << std::endl;
    }
}

static bool printed = false;

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

    printed = true;		// Suppress printing of co-ordinates.

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
		    if (! printed)
			printf("Axis[%d] %5.2f %5.2f %5.2f\n",
				axis, p[X], p[Y], p[Z]);
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
			// as you view each face from the outside loooking in.
			p[coord2] = (2*face -1) * (2*k-1) * lenB;
			glVertex3fv (p);	// Draw 1 of 4 vertices.
			if (! printed)
			    printf("Edge[%d] %5.2f %5.2f %5.2f\n",
				    edge, p[X], p[Y], p[Z]);
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
		// if (! printed)
		    // printf("Corner[%d] %5.2f %5.2f %5.2f\n",
			    // face, x, y, z);
	    }
	}
    } // END (face, 2)

    glEnd();

    printed = true;
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
    // Convert Y co-ordinate to OpenGL convention (zero at bottom of window).
    game->handleMouseEvent (ButtonDown, e->button(),
				e->pos().x(), height() - e->pos().y());
}


void GameGLView::mouseReleaseEvent(QMouseEvent* e)
{
    // Convert Y co-ordinate to OpenGL convention (zero at bottom of window).
    game->handleMouseEvent (ButtonUp, e->button(),
				e->pos().x(), height() - e->pos().y());
}

// End gameglview.cpp
