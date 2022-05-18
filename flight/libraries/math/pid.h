/**
 ******************************************************************************
 * @file       pid.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Methods to work with PID structure
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

#ifndef PID_H
#define PID_H

#include "mathmisc.h"

// !
struct pid {
    float p;
    float i;
    float d;
    float iLim;
    float iAccumulator;
    float lastErr;
    float lastDer;
};

// pid2 structure for a PID+setpoint weighting, anti-windup and filtered derivative control
struct pid2 {
    float   u0;   // Initial value of r & y - for bumpless transfer
    float   va;   // Actuator settings for scaling
    float   vb;
    float   kp;   // proportional gain
    float   bi;   // Integral gain . dT
    float   ad;   // Filtered factor for derivative calculation
    float   bd;   // Constant for derivative calculation
    float   br;   // Time constant for integral calculation
    float   beta; // Scalar for proportional factor
    float   yold; // t-1 y value for Integral calculation
    float   P;    // Latest calculated P, I & D values
    float   I;
    float   D;
    uint8_t reconfigure;
};

typedef struct pid_scaler_s {
    float p;
    float i;
    float d;
} pid_scaler;

// ! Methods to use the pid structures
float pid_apply(struct pid *pid, const float err, float dT);
float pid_apply_setpoint(struct pid *pid, const pid_scaler *scaler, const float setpoint, const float measured, float dT, bool meas_based_d_term);
void pid_zero(struct pid *pid);
void pid_configure(struct pid *pid, float p, float i, float d, float iLim);
void pid_configure_derivative(float cutoff, float gamma);

// Methods for use with pid2 structure
void pid2_configure(struct pid2 *pid, float kp, float ki, float kd, float Tf, float kt, float dT, float beta, float u0, float va, float vb);
void pid2_transfer(struct pid2 *pid, float u0);
float pid2_apply(struct pid2 *pid, const float r, const float y, float ulow, float uhigh);

#endif /* PID_H */
