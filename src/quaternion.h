/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
