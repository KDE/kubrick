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

/*
  A little library to do quaternion arithmetic.  Adapted from C to C++.
  Acknowledgements and thanks to John Darrington and the Gnubik package.
*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "quaternion.h"

Quaternion::Quaternion ()
{
    quaternionSetIdentity();
}


void Quaternion::quaternionSetIdentity ()
{
    w = 1.0; 
    x = 0.0; 
    y = 0.0; 
    z = 0.0; 
}


void Quaternion::quaternionAddRotation
			(const double axis[], const double degrees)
{
    // If the axis-vector and quaternion are both normalised,
    // then the new quaternion will also be normalised.

    Quaternion q;
    double radians = degrees * M_PI / 180.0;
    double s = sin (radians / 2.0);

    q.w = cos (radians / 2.0);
    q.x = axis [0] * s;
    q.y = axis [1] * s;
    q.z = axis [2] * s;

    quaternionPreMultiply (this, &q);
} 


void Quaternion::quaternionPreMultiply (Quaternion * q1, const Quaternion * q2)
{
    double s1 = q1->w;
    double x1 = q1->x;
    double y1 = q1->y;
    double z1 = q1->z;

    double s2 = q2->w;
    double x2 = q2->x;
    double y2 = q2->y;
    double z2 = q2->z;

    double dot_product;
    double cross_product [3]; 
/* 
    printf("Q mult\n");
    quaternionPrint(q1);
    quaternionPrint(q2);
*/
    dot_product = x1*x2 + y1*y2 + z1*z2; 
/* 
    printf("Dot product is %f\n",dot_product);
*/
    cross_product [0] = y1*z2 - z1*y2;
    cross_product [1] = z1*x2 - x1*z2;
    cross_product [2] = x1*y2 - y1*x2;
/* 
    printf("Cross product is %f, %f, %f\n",cross_product[0],
    cross_product[1],
    cross_product[2]);
*/
    q1->w = s1*s2 - dot_product;
    q1->x = s1*x2 + s2*x1 + cross_product[0];
    q1->y = s1*y2 + s2*y1 + cross_product[1];
    q1->z = s1*z2 + s2*z1 + cross_product[2];
}


void Quaternion::quaternionToMatrix (Matrix M)
{
    double ww = w * w;
    double xx = x * x;
    double yy = y * y;
    double zz = z * z;

    double wx = w * x;
    double wy = w * y;
    double wz = w * z;

    double xy = x * y;
    double yz = y * z;
    double zx = z * x;

    int    dim = DIMENSIONS;

    /* Diagonal */
    M [0 + dim * 0] = ww + xx - yy - zz; // If q is normalised, 1 - 2*y2 - 2*z2.
    M [1 + dim * 1] = ww - xx + yy - zz; // etc.
    M [2 + dim * 2] = ww - xx - yy + zz; // etc.
    M [3 + dim * 3] = ww + xx + yy + zz; // If q is normalised, this is 1.0.

    /* Last row */
    M [0 + dim * 3] = 0.0;
    M [1 + dim * 3] = 0.0;
    M [2 + dim * 3] = 0.0;

    /* Last Column */
    M [3 + dim * 0] = 0.0;
    M [3 + dim * 1] = 0.0;
    M [3 + dim * 2] = 0.0;

    /* Others */
    M [0 + dim * 1] = 2.0 * (xy + wz);
    M [0 + dim * 2] = 2.0 * (zx - wy);
    M [1 + dim * 2] = 2.0 * (yz + wx);

    M [1 + dim * 0] = 2.0 * (xy - wz);
    M [2 + dim * 0] = 2.0 * (zx + wy);
    M [2 + dim * 1] = 2.0 * (yz - wx);
}


void Quaternion::quaternionPrint()
{
    printf ("(%0.2f, %0.2f, %0.2f, %0.2f)\n", w, x, y, z);
}
