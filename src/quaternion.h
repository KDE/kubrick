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

#ifndef QUATERNION_H
#define QUATERNION_H

/*
  A little library to do quaternion arithmetic.  Adapted from C to C++.
  Acknowledgements and thanks to John Darrington and the Gnubik package.
*/

#define DIMENSIONS 4

typedef float Matrix [DIMENSIONS * DIMENSIONS];

class Quaternion
{
public:
    Quaternion ();
    Quaternion (double realPart, double iPart, double jPart, double kPart);

    void quaternionSetIdentity();
    void quaternionAddRotation (const double axis[3], const double degrees);
    void quaternionPreMultiply (Quaternion * q1, const Quaternion * q2) const;
    void quaternionToMatrix (Matrix M) const;
    void quaternionInvert();
    void quaternionRotateVector (double axis[3]) const;
    void quaternionPrint() const;

private:
    double w;			// The "real" part.
    double x;			// The "i" part.
    double y;			// The "j" part.
    double z;			// The "k" part.
};

#endif	// QUATERNION_H
