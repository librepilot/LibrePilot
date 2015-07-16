/**
 ******************************************************************************
 *
 * @file       calibrationutils.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 *
 * @brief      Utilities for calibration. Ellipsoid and polynomial fit algorithms
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
 * @{
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

#ifndef CALIBRATIONUTILS_H
#define CALIBRATIONUTILS_H
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/Dense>
#include <Eigen/LU>
#include <QList>
namespace OpenPilot {
class CalibrationUtils {
public:
    struct EllipsoidCalibrationResult {
        Eigen::Matrix3f CalibrationMatrix;
        Eigen::Vector3f Scale;
        Eigen::Vector3f Bias;
    };
    static bool EllipsoidCalibration(Eigen::VectorXf *samplesX, Eigen::VectorXf *samplesY, Eigen::VectorXf *samplesZ,
                                     float nominalRange,
                                     EllipsoidCalibrationResult *result,
                                     bool fitAlongXYZ);
    static bool PolynomialCalibration(Eigen::VectorXf *samplesX, Eigen::VectorXf *samplesY, int degree, Eigen::Ref<Eigen::VectorXf> result, const double maxRelativeError);

    static void ComputePoly(Eigen::VectorXf *samplesX, Eigen::VectorXf *polynomial, Eigen::VectorXf *polyY);
    static float ComputeSigma(Eigen::VectorXf *samplesY);

    static int SixPointInConstFieldCal(double ConstMag, double x[6], double y[6], double z[6], double S[3], double b[3]);
    static double listMean(QList<double> list);
    static double listVar(QList<double> list);
private:
    static void EllipsoidFit(Eigen::VectorXf *samplesX, Eigen::VectorXf *samplesY, Eigen::VectorXf *samplesZ,
                             Eigen::Vector3f *center,
                             Eigen::VectorXf *radii,
                             Eigen::MatrixXf *evecs, bool fitAlongXYZ);

    static int LinearEquationsSolve(int nDim, double *pfMatr, double *pfVect, double *pfSolution);
};
}
#endif // CALIBRATIONUTILS_H
