/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup AltitudeModule Altitude Module
 * @brief Communicate with BMP085 and update @ref BaroSensor "BaroSensor UAV Object"
 * @{
 *
 * @file       altitude.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Altitude module, handles temperature and pressure readings from BMP085
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

/**
 * Output object: BaroSensor
 *
 * This module will periodically update the value of the BaroSensor object.
 *
 */

#include <openpilot.h>

#include "hwsettings.h"
#include "altitude.h"
#if defined(PIOS_INCLUDE_BMP085)
#include "barosensor.h" // object that will be updated by the module
#endif
#if defined(PIOS_INCLUDE_HCSR04)
#include "sonaraltitude.h" // object that will be updated by the module
#endif
#include "taskinfo.h"

// Private constants
#define STACK_SIZE_BYTES 500
#define TASK_PRIORITY    (tskIDLE_PRIORITY + 1)
#define UPDATE_PERIOD    50

// Private types

// Private variables
static xTaskHandle taskHandle;

// down sampling variables
#if defined(PIOS_INCLUDE_BMP085)
#define alt_ds_size 4
static int32_t alt_ds_temp = 0;
static int32_t alt_ds_pres = 0;
static int alt_ds_count    = 0;
#endif

static bool altitudeEnabled;
static uint8_t hwsettings_rcvrport;;

// Private functions
static void altitudeTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AltitudeStart()
{
    if (altitudeEnabled) {
#if defined(PIOS_INCLUDE_BMP085)
        BaroSensorInitialize();
#endif
#if defined(PIOS_INCLUDE_HCSR04)
        SonarAltitudeInitialize();
#endif

        // Start main task
        xTaskCreate(altitudeTask, (signed char *)"Altitude", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
        PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_ALTITUDE, taskHandle);
        return 0;
    }
    return -1;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AltitudeInitialize()
{
#ifdef MODULE_ALTITUDE_BUILTIN
    altitudeEnabled = 1;
#else
    uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
    HwSettingsOptionalModulesGet(optionalModules);
    if (optionalModules[HWSETTINGS_OPTIONALMODULES_ALTITUDE] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        altitudeEnabled = 1;
    } else {
        altitudeEnabled = 0;
    }
#endif

#if defined(PIOS_INCLUDE_BMP085)
    // init down-sampling data
    alt_ds_temp  = 0;
    alt_ds_pres  = 0;
    alt_ds_count = 0;
#endif
    HwSettingsCC_RcvrPortGet(&hwsettings_rcvrport);
    return 0;
}
MODULE_INITCALL(AltitudeInitialize, AltitudeStart);
/**
 * Module thread, should not return.
 */
static void altitudeTask(__attribute__((unused)) void *parameters)
{
    portTickType lastSysTime;

#if defined(PIOS_INCLUDE_HCSR04)
    SonarAltitudeData sonardata;
    int32_t value = 0, timeout = 5;
    float coeff   = 0.25, height_out = 0, height_in = 0;
    if (hwsettings_rcvrport == HWSETTINGS_CC_RCVRPORT_DISABLED) {
        PIOS_HCSR04_Trigger();
    }
#endif
#if defined(PIOS_INCLUDE_BMP085)
    BaroSensorData data;
    PIOS_BMP085_Init();
#endif

    // Main task loop
    lastSysTime = xTaskGetTickCount();
    while (1) {
#if defined(PIOS_INCLUDE_HCSR04)
        // Compute the current altitude
        if (hwsettings_rcvrport == HWSETTINGS_CC_RCVRPORT_DISABLED) {
            if (PIOS_HCSR04_Completed()) {
                value = PIOS_HCSR04_Get();
                // from 3.4cm to 5.1m
                if ((value > 100) && (value < 15000)) {
                    height_in  = value * 0.00034f / 2.0f;
                    height_out = (height_out * (1 - coeff)) + (height_in * coeff);
                    sonardata.Altitude = height_out; // m/us
                }

                // Update the AltitudeActual UAVObject
                SonarAltitudeSet(&sonardata);
                timeout = 5;
                PIOS_HCSR04_Trigger();
            }
            if (!(timeout--)) {
                // retrigger
                timeout = 5;
                PIOS_HCSR04_Trigger();
            }
        }
#endif /* if defined(PIOS_INCLUDE_HCSR04) */
#if defined(PIOS_INCLUDE_BMP085)
        // Update the temperature data
        PIOS_BMP085_StartADC(TemperatureConv);
#ifdef PIOS_BMP085_HAS_GPIOS
        xSemaphoreTake(PIOS_BMP085_EOC, portMAX_DELAY);
#else
        vTaskDelay(5 / portTICK_RATE_MS);
#endif
        PIOS_BMP085_ReadADC();
        alt_ds_temp += PIOS_BMP085_GetTemperature();

        // Update the pressure data
        PIOS_BMP085_StartADC(PressureConv);
#ifdef PIOS_BMP085_HAS_GPIOS
        xSemaphoreTake(PIOS_BMP085_EOC, portMAX_DELAY);
#else
        vTaskDelay(26 / portTICK_RATE_MS);
#endif
        PIOS_BMP085_ReadADC();
        alt_ds_pres += PIOS_BMP085_GetPressure();

        if (++alt_ds_count >= alt_ds_size) {
            alt_ds_count     = 0;

            // Convert from 1/10ths of degC to degC
            data.Temperature = alt_ds_temp / (10.0 * alt_ds_size);
            alt_ds_temp   = 0;

            // Convert from Pa to kPa
            data.Pressure = alt_ds_pres / (1000.0f * alt_ds_size);
            alt_ds_pres   = 0;

            // Compute the current altitude (all pressures in kPa)
            data.Altitude = 44330.0 * (1.0 - powf((data.Pressure / (BMP085_P0 / 1000.0)), (1.0 / 5.255)));

            // Update the AltitudeActual UAVObject
            BaroSensorSet(&data);
        }
#endif /* if defined(PIOS_INCLUDE_BMP085) */

        // Delay until it is time to read the next sample
        vTaskDelayUntil(&lastSysTime, UPDATE_PERIOD / portTICK_RATE_MS);
    }
}

/**
 * @}
 * @}
 */
