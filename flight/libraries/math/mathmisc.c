/**
 ******************************************************************************
 * @addtogroup OpenPilot Math Utilities
 * @{
 * @addtogroup Reuseable math functions
 * @{
 *
 * @file       mathmisc.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Reuseable math functions
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

#include <mathmisc.h>

void pseudo_windowed_variance_init(pw_variance_t *variance, int32_t window_size)
{
    variance->new_sma  = 0.0f;
    variance->new_smsa = 0.0f;
    variance->p1 = 1.0f / (float)window_size;
    variance->p2 = 1.0f - variance->p1;
}

float pseudo_windowed_variance_get(pw_variance_t *variance)
{
    return variance->new_smsa - variance->new_sma * variance->new_sma;
}
