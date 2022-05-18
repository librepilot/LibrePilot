/**
 ******************************************************************************
 *
 * @file       CoordinateConversions.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      General conversions with different coordinate systems.
 *             - all angles in deg
 *             - distances in meters
 *             - altitude above WGS-84 elipsoid
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdint.h>
#include <pios_math.h>
#include "inc/CoordinateConversions.h"

#define MIN_ALLOWABLE_MAGNITUDE 1e-30f

// Equatorial Radius
#define equatorial_radius       6378137.0d

// Eccentricity
#define eccentricity            8.1819190842622e-2d
#define equatorial_radius_sq    (equatorial_radius * equatorial_radius)
#define eccentricity_sq         (eccentricity * eccentricity)

// ****** convert Lat,Lon,Alt to ECEF  ************
void LLA2ECEF(const int32_t LLAi[3], float ECEF[3])
{
    double sinLat, sinLon, cosLat, cosLon;
    double N;
    double LLA[3] = {
        ((double)LLAi[0]) * 1e-7d,
        ((double)LLAi[1]) * 1e-7d,
        ((double)LLAi[2]) * 1e-4d
    };

    sinLat = sin(DEG2RAD_D(LLA[0]));
    sinLon = sin(DEG2RAD_D(LLA[1]));
    cosLat = cos(DEG2RAD_D(LLA[0]));
    cosLon = cos(DEG2RAD_D(LLA[1]));

    N = equatorial_radius / sqrt(1.0d - eccentricity_sq * sinLat * sinLat); // prime vertical radius of curvature

    ECEF[0] = (float)((N + LLA[2]) * cosLat * cosLon);
    ECEF[1] = (float)((N + LLA[2]) * cosLat * sinLon);
    ECEF[2] = (float)(((1.0d - eccentricity_sq) * N + LLA[2]) * sinLat);
}

// ****** convert ECEF to Lat,Lon,Alt (ITERATIVE!) *********
void ECEF2LLA(const float ECEF[3], int32_t LLA[3])
{
/*    b = math.sqrt( asq * (1-esq) )
       bsq = b*b

       ep = math.sqrt((asq - bsq)/bsq)
       p = math.sqrt( math.pow(x,2) + math.pow(y,2) )
       th = math.atan2(a*z, b*p)

       lon = math.atan2(y,x)
       lat = math.atan2( (z + ep*ep *b * math.pow(math.sin(th),3) ), (p - esq*a*math.pow(math.cos(th),3)) )
       N = a/( math.sqrt(1-esq*math.pow(math.sin(lat),2)) )
       alt = p / math.cos(lat) - N*/

    const double x   = ECEF[0], y = ECEF[1], z = ECEF[2];

    const double b   = sqrt(equatorial_radius_sq * (1 - eccentricity_sq));
    const double bsq = b * b;
    const double ep  = sqrt((equatorial_radius_sq - bsq) / bsq);

    const double p   = sqrt(x * x + y * y);
    const double th  = atan2(equatorial_radius * z, b * p);

    double lon = atan2(y, x);

    const double lat = atan2(
        (z + ep * ep * b * pow(sin(th), 3)),
        (p - eccentricity_sq * equatorial_radius * pow(cos(th), 3))
        );
    const double N   = equatorial_radius / (sqrt(1 - eccentricity_sq * pow(sin(lat), 2)));
    const double alt = p / cos(lat) - N;

    LLA[0] = (int32_t)(RAD2DEG_D(lat) * 1e7d);
    LLA[1] = (int32_t)(RAD2DEG_D(lon) * 1e7d) % ((int32_t)(180 * 1e7d));
    LLA[2] = (int32_t)(alt * 1e4d);
}

// ****** find ECEF to NED rotation matrix ********
void RneFromLLA(const int32_t LLAi[3], float Rne[3][3])
{
    float sinLat, sinLon, cosLat, cosLon;

    sinLat    = sinf(DEG2RAD((float)LLAi[0] * 1e-7f));
    sinLon    = sinf(DEG2RAD((float)LLAi[1] * 1e-7f));
    cosLat    = cosf(DEG2RAD((float)LLAi[0] * 1e-7f));
    cosLon    = cosf(DEG2RAD((float)LLAi[1] * 1e-7f));

    Rne[0][0] = -sinLat * cosLon;
    Rne[0][1] = -sinLat * sinLon;
    Rne[0][2] = cosLat;
    Rne[1][0] = -sinLon;
    Rne[1][1] = cosLon;
    Rne[1][2] = 0;
    Rne[2][0] = -cosLat * cosLon;
    Rne[2][1] = -cosLat * sinLon;
    Rne[2][2] = -sinLat;
}

// ****** find roll, pitch, yaw from quaternion ********
void Quaternion2RPY(const float q[4], float rpy[3])
{
    float R13, R11, R12, R23, R33;
    float q0s = q[0] * q[0];
    float q1s = q[1] * q[1];
    float q2s = q[2] * q[2];
    float q3s = q[3] * q[3];

    R13    = 2.0f * (q[1] * q[3] - q[0] * q[2]);
    R11    = q0s + q1s - q2s - q3s;
    R12    = 2.0f * (q[1] * q[2] + q[0] * q[3]);
    R23    = 2.0f * (q[2] * q[3] + q[0] * q[1]);
    R33    = q0s - q1s - q2s + q3s;

    rpy[1] = RAD2DEG(asinf(-R13)); // pitch always between -pi/2 to pi/2
    rpy[2] = RAD2DEG(atan2f(R12, R11));
    rpy[0] = RAD2DEG(atan2f(R23, R33));

    // TODO: consider the cases where |R13| ~= 1, |pitch| ~= pi/2
}

// ****** find quaternion from roll, pitch, yaw ********
void RPY2Quaternion(const float rpy[3], float q[4])
{
    float phi, theta, psi;
    float cphi, sphi, ctheta, stheta, cpsi, spsi;

    phi    = DEG2RAD(rpy[0] / 2);
    theta  = DEG2RAD(rpy[1] / 2);
    psi    = DEG2RAD(rpy[2] / 2);
    cphi   = cosf(phi);
    sphi   = sinf(phi);
    ctheta = cosf(theta);
    stheta = sinf(theta);
    cpsi   = cosf(psi);
    spsi   = sinf(psi);

    q[0]   = cphi * ctheta * cpsi + sphi * stheta * spsi;
    q[1]   = sphi * ctheta * cpsi - cphi * stheta * spsi;
    q[2]   = cphi * stheta * cpsi + sphi * ctheta * spsi;
    q[3]   = cphi * ctheta * spsi - sphi * stheta * cpsi;

    if (q[0] < 0) { // q0 always positive for uniqueness
        q[0] = -q[0];
        q[1] = -q[1];
        q[2] = -q[2];
        q[3] = -q[3];
    }
}

// ** Find Rbe, that rotates a vector from earth fixed to body frame, from quaternion **
void Quaternion2R(float q[4], float Rbe[3][3])
{
    const float q0s = q[0] * q[0], q1s = q[1] * q[1], q2s = q[2] * q[2], q3s = q[3] * q[3];

    Rbe[0][0] = q0s + q1s - q2s - q3s;
    Rbe[0][1] = 2 * (q[1] * q[2] + q[0] * q[3]);
    Rbe[0][2] = 2 * (q[1] * q[3] - q[0] * q[2]);
    Rbe[1][0] = 2 * (q[1] * q[2] - q[0] * q[3]);
    Rbe[1][1] = q0s - q1s + q2s - q3s;
    Rbe[1][2] = 2 * (q[2] * q[3] + q[0] * q[1]);
    Rbe[2][0] = 2 * (q[1] * q[3] + q[0] * q[2]);
    Rbe[2][1] = 2 * (q[2] * q[3] - q[0] * q[1]);
    Rbe[2][2] = q0s - q1s - q2s + q3s;
}


// ** Find first row of Rbe, that rotates a vector from earth fixed to body frame, from quaternion **
// ** This vector corresponds to the fuselage/roll vector xB **
void QuaternionC2xB(const float q0, const float q1, const float q2, const float q3, float x[3])
{
    const float q0s = q0 * q0, q1s = q1 * q1, q2s = q2 * q2, q3s = q3 * q3;

    x[0] = q0s + q1s - q2s - q3s;
    x[1] = 2 * (q1 * q2 + q0 * q3);
    x[2] = 2 * (q1 * q3 - q0 * q2);
}


void Quaternion2xB(const float q[4], float x[3])
{
    QuaternionC2xB(q[0], q[1], q[2], q[3], x);
}


// ** Find second row of Rbe, that rotates a vector from earth fixed to body frame, from quaternion **
// ** This vector corresponds to the spanwise/pitch vector yB **
void QuaternionC2yB(const float q0, const float q1, const float q2, const float q3, float y[3])
{
    const float q0s = q0 * q0, q1s = q1 * q1, q2s = q2 * q2, q3s = q3 * q3;

    y[0] = 2 * (q1 * q2 - q0 * q3);
    y[1] = q0s - q1s + q2s - q3s;
    y[2] = 2 * (q2 * q3 + q0 * q1);
}


void Quaternion2yB(const float q[4], float y[3])
{
    QuaternionC2yB(q[0], q[1], q[2], q[3], y);
}


// ** Find third row of Rbe, that rotates a vector from earth fixed to body frame, from quaternion **
// ** This vector corresponds to the vertical/yaw vector zB **
void QuaternionC2zB(const float q0, const float q1, const float q2, const float q3, float z[3])
{
    const float q0s = q0 * q0, q1s = q1 * q1, q2s = q2 * q2, q3s = q3 * q3;

    z[0] = 2 * (q1 * q3 + q0 * q2);
    z[1] = 2 * (q2 * q3 - q0 * q1);
    z[2] = q0s - q1s - q2s + q3s;
}


void Quaternion2zB(const float q[4], float z[3])
{
    QuaternionC2zB(q[0], q[1], q[2], q[3], z);
}


// ****** Express LLA in a local NED Base Frame ********
void LLA2Base(const int32_t LLAi[3], const float BaseECEF[3], float Rne[3][3], float NED[3])
{
    float ECEF[3];

    LLA2ECEF(LLAi, ECEF);
    ECEF2Base(ECEF, BaseECEF, Rne, NED);
}

// ****** Express LLA in a local NED Base Frame ********
void Base2LLA(const float NED[3], const float BaseECEF[3], float Rne[3][3], int32_t LLAi[3])
{
    float ECEF[3];

    Base2ECEF(NED, BaseECEF, Rne, ECEF);
    ECEF2LLA(ECEF, LLAi);
}
// ****** Express ECEF in a local NED Base Frame ********
void ECEF2Base(const float ECEF[3], const float BaseECEF[3], float Rne[3][3], float NED[3])
{
    float diff[3];

    diff[0] = (ECEF[0] - BaseECEF[0]);
    diff[1] = (ECEF[1] - BaseECEF[1]);
    diff[2] = (ECEF[2] - BaseECEF[2]);

    NED[0]  = Rne[0][0] * diff[0] + Rne[0][1] * diff[1] + Rne[0][2] * diff[2];
    NED[1]  = Rne[1][0] * diff[0] + Rne[1][1] * diff[1] + Rne[1][2] * diff[2];
    NED[2]  = Rne[2][0] * diff[0] + Rne[2][1] * diff[1] + Rne[2][2] * diff[2];
}

// ****** Express ECEF in a local NED Base Frame ********
void Base2ECEF(const float NED[3], const float BaseECEF[3], float Rne[3][3], float ECEF[3])
{
    float diff[3];

    diff[0] = Rne[0][0] * NED[0] + Rne[1][0] * NED[1] + Rne[2][0] * NED[2];
    diff[1] = Rne[0][1] * NED[0] + Rne[1][1] * NED[1] + Rne[2][1] * NED[2];
    diff[2] = Rne[0][2] * NED[0] + Rne[1][2] * NED[1] + Rne[2][2] * NED[2];


    ECEF[0] = diff[0] + BaseECEF[0];
    ECEF[1] = diff[1] + BaseECEF[1];
    ECEF[2] = diff[2] + BaseECEF[2];
}


// ****** convert Rotation Matrix to Quaternion ********
// ****** if R converts from e to b, q is rotation from e to b ****
void R2Quaternion(float R[3][3], float q[4])
{
    float m[4], mag;
    uint8_t index, i;

    m[0]  = 1 + R[0][0] + R[1][1] + R[2][2];
    m[1]  = 1 + R[0][0] - R[1][1] - R[2][2];
    m[2]  = 1 - R[0][0] + R[1][1] - R[2][2];
    m[3]  = 1 - R[0][0] - R[1][1] + R[2][2];

    // find maximum divisor
    index = 0;
    mag   = m[0];
    for (i = 1; i < 4; i++) {
        if (m[i] > mag) {
            mag   = m[i];
            index = i;
        }
    }
    mag = 2 * sqrtf(mag);

    if (index == 0) {
        q[0] = mag / 4;
        q[1] = (R[1][2] - R[2][1]) / mag;
        q[2] = (R[2][0] - R[0][2]) / mag;
        q[3] = (R[0][1] - R[1][0]) / mag;
    } else if (index == 1) {
        q[1] = mag / 4;
        q[0] = (R[1][2] - R[2][1]) / mag;
        q[2] = (R[0][1] + R[1][0]) / mag;
        q[3] = (R[0][2] + R[2][0]) / mag;
    } else if (index == 2) {
        q[2] = mag / 4;
        q[0] = (R[2][0] - R[0][2]) / mag;
        q[1] = (R[0][1] + R[1][0]) / mag;
        q[3] = (R[1][2] + R[2][1]) / mag;
    } else {
        q[3] = mag / 4;
        q[0] = (R[0][1] - R[1][0]) / mag;
        q[1] = (R[0][2] + R[2][0]) / mag;
        q[2] = (R[1][2] + R[2][1]) / mag;
    }

    // q0 positive, i.e. angle between pi and -pi
    if (q[0] < 0) {
        q[0] = -q[0];
        q[1] = -q[1];
        q[2] = -q[2];
        q[3] = -q[3];
    }
}

// ****** Rotation Matrix from Two Vector Directions ********
// ****** given two vector directions (v1 and v2) known in two frames (b and e) find Rbe ***
// ****** solution is approximate if can't be exact ***
uint8_t RotFrom2Vectors(const float v1b[3], const float v1e[3], const float v2b[3], const float v2e[3], float Rbe[3][3])
{
    float Rib[3][3], Rie[3][3];
    float mag;
    uint8_t i, j, k;

    // identity rotation in case of error
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            Rbe[i][j] = 0;
        }
        Rbe[i][i] = 1;
    }

    // The first rows of rot matrices chosen in direction of v1
    mag = VectorMagnitude(v1b);
    if (fabsf(mag) < MIN_ALLOWABLE_MAGNITUDE) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        Rib[0][i] = v1b[i] / mag;
    }

    mag = VectorMagnitude(v1e);
    if (fabsf(mag) < MIN_ALLOWABLE_MAGNITUDE) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        Rie[0][i] = v1e[i] / mag;
    }

    // The second rows of rot matrices chosen in direction of v1xv2
    CrossProduct(v1b, v2b, &Rib[1][0]);
    mag = VectorMagnitude(&Rib[1][0]);
    if (fabsf(mag) < MIN_ALLOWABLE_MAGNITUDE) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        Rib[1][i] = Rib[1][i] / mag;
    }

    CrossProduct(v1e, v2e, &Rie[1][0]);
    mag = VectorMagnitude(&Rie[1][0]);
    if (fabsf(mag) < MIN_ALLOWABLE_MAGNITUDE) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        Rie[1][i] = Rie[1][i] / mag;
    }

    // The third rows of rot matrices are XxY (Row1xRow2)
    CrossProduct(&Rib[0][0], &Rib[1][0], &Rib[2][0]);
    CrossProduct(&Rie[0][0], &Rie[1][0], &Rie[2][0]);

    // Rbe = Rbi*Rie = Rib'*Rie
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            Rbe[i][j] = 0;
            for (k = 0; k < 3; k++) {
                Rbe[i][j] += Rib[k][i] * Rie[k][j];
            }
        }
    }

    return 1;
}

void Rv2Rot(float Rv[3], float R[3][3])
{
    // Compute rotation matrix from a rotation vector
    // To save .text space, uses Quaternion2R()
    float q[4];

    float angle = VectorMagnitude(Rv);

    if (angle <= 0.00048828125f) {
        // angle < sqrt(2*machine_epsilon(float)), so flush cos(x) to 1.0f
        q[0] = 1.0f;

        // and flush sin(x/2)/x to 0.5
        q[1] = 0.5f * Rv[0];
        q[2] = 0.5f * Rv[1];
        q[3] = 0.5f * Rv[2];
        // This prevents division by zero, while retaining full accuracy
    } else {
        q[0] = cosf(angle * 0.5f);
        float scale = sinf(angle * 0.5f) / angle;
        q[1] = scale * Rv[0];
        q[2] = scale * Rv[1];
        q[3] = scale * Rv[2];
    }

    Quaternion2R(q, R);
}

// ****** Vector Cross Product ********
void CrossProduct(const float v1[3], const float v2[3], float result[3])
{
    result[0] = v1[1] * v2[2] - v2[1] * v1[2];
    result[1] = v2[0] * v1[2] - v1[0] * v2[2];
    result[2] = v1[0] * v2[1] - v2[0] * v1[1];
}

// ****** Vector Magnitude ********
float VectorMagnitude(const float v[3])
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

/**
 * @brief Compute the inverse of a quaternion
 * @param [in][out] q The matrix to invert
 */
void quat_inverse(float q[4])
{
    q[1] = -q[1];
    q[2] = -q[2];
    q[3] = -q[3];
}

/**
 * @brief Duplicate a quaternion
 * @param[in] q quaternion in
 * @param[out] qnew quaternion to copy to
 */
void quat_copy(const float q[4], float qnew[4])
{
    qnew[0] = q[0];
    qnew[1] = q[1];
    qnew[2] = q[2];
    qnew[3] = q[3];
}

/**
 * @brief Multiply two quaternions into a third
 * @param[in] q1 First quaternion
 * @param[in] q2 Second quaternion
 * @param[out] qout Output quaternion
 */
void quat_mult(const float q1[4], const float q2[4], float qout[4])
{
    qout[0] = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
    qout[1] = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2];
    qout[2] = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
    qout[3] = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
}

/**
 * @brief Rotate a vector by a rotation matrix
 * @param[in] R a three by three rotation matrix (first index is row)
 * @param[in] vec the source vector
 * @param[out] vec_out the output vector
 */
void rot_mult(float R[3][3], const float vec[3], float vec_out[3])
{
    vec_out[0] = R[0][0] * vec[0] + R[0][1] * vec[1] + R[0][2] * vec[2];
    vec_out[1] = R[1][0] * vec[0] + R[1][1] * vec[1] + R[1][2] * vec[2];
    vec_out[2] = R[2][0] * vec[0] + R[2][1] * vec[1] + R[2][2] * vec[2];
}
