/*
 * Copyright (C) 2016 The LibrePilot Project
 * Contact: http://www.librepilot.org
 *
 * This file is part of LibrePilot GCS.
 *
 * LibrePilot GCS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LibrePilot GCS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibrePilot GCS.  If not, see <http://www.gnu.org/licenses/>.
 */

// ***************************
// Common javascript uav utils
// ***************************

// Settings
//
// Control
.import UAVTalk.VtolPathFollowerSettings 1.0 as VtolPathFollowerSettings

// Navigation
.import UAVTalk.HomeLocation 1.0 as HomeLocation
.import UAVTalk.TakeOffLocation 1.0 as TakeOffLocation

// Sensors
.import UAVTalk.FlightBatterySettings 1.0 as FlightBatterySettings

// State
.import UAVTalk.RevoSettings 1.0 as RevoSettings

// System
.import UAVTalk.HwSettings 1.0 as HwSettings
.import UAVTalk.SystemSettings 1.0 as SystemSettings

// Data
//
// Control
.import UAVTalk.FlightStatus 1.0 as FlightStatus
.import UAVTalk.ManualControlCommand 1.0 as ManualControlCommand
.import UAVTalk.StabilizationDesired 1.0 as StabilizationDesired
.import UAVTalk.VelocityDesired 1.0 as VelocityDesired

// Navigation
.import UAVTalk.PathDesired 1.0 as PathDesired
.import UAVTalk.WaypointActive 1.0 as WaypointActive

// Sensors
.import UAVTalk.FlightBatteryState 1.0 as FlightBatteryState
.import UAVTalk.GPSPositionSensor 1.0 as GPSPositionSensor
.import UAVTalk.GPSSatellites 1.0 as GPSSatellites

// State
.import UAVTalk.AttitudeState 1.0 as AttitudeState
.import UAVTalk.MagState 1.0 as MagState
.import UAVTalk.NedAccel 1.0 as NedAccel
.import UAVTalk.PositionState 1.0 as PositionState
.import UAVTalk.VelocityState 1.0 as VelocityState

// System
.import UAVTalk.OPLinkStatus 1.0 as OPLinkStatus
.import UAVTalk.ReceiverStatus 1.0 as ReceiverStatus
.import UAVTalk.SystemAlarms 1.0 as SystemAlarms

Qt.include("common.js")

// End Import


/*
 * State functions
 *
*/

function attitude() {
    return Qt.vector3d(attitudeState.roll, attitudeState.pitch, attitudeState.yaw);
}

function attitudeRoll() {
    return attitudeState.roll;
}

function attitudePitch() {
    return attitudeState.pitch;
}

function attitudeYaw() {
    return attitudeState.yaw;
}

function currentVelocity() {
    return Math.sqrt(Math.pow(velocityState.north, 2) + Math.pow(velocityState.east, 2));
}

function positionStateDown() {
    return positionState.down;
}

function velocityStateDown() {
    return velocityState.down;
}

function nedAccelDown() {
    return nedAccel.down;
}

function position() {
    switch(gpsPositionSensor.status) {
    case GPSPositionSensor.Status.Fix2D:
    case GPSPositionSensor.Status.Fix3D:
        return gpsPosition();
    case GPSPositionSensor.Status.NoFix:
    case GPSPositionSensor.Status.NoGPS:
    default:
        return (homeLocation.set == HomeLocation.Set.True) ? homePosition() : defaultPosition();
    }
}

function gpsPosition() {
    return Qt.vector3d(
        gpsPositionSensor.longitude / 10000000.0,
        gpsPositionSensor.latitude / 10000000.0,
        gpsPositionSensor.altitude);
}

function homePosition() {
    return Qt.vector3d(
        homeLocation.longitude / 10000000.0,
        homeLocation.latitude / 10000000.0,
        homeLocation.altitude);
}

function defaultPosition() {
    return Qt.vector3d(pfdContext.longitude, pfdContext.latitude, pfdContext.altitude);
}

/*
 * System
 *
*/
function isCC3D() {
    // Hack: detect Coptercontrol with mem free
    return (freeMemoryBytes() < 3096);
}

function frameType() {
    return ["FixedWing", "FixedWingElevon", "FixedWingVtail", "VTOL", "HeliCP", "QuadX", "QuadP",
            "Hexa+", "Octo+", "Custom", "HexaX", "HexaH", "OctoV", "OctoCoaxP", "OctoCoaxX", "OctoX", "HexaCoax",
            "Tricopter", "GroundVehicleCar", "GroundVehicleDiff", "GroundVehicleMoto"][systemSettings.airframeType]
}

function isVtolOrMultirotor() {
    return ((systemSettings.airframeType > SystemSettings.AirframeType.FixedWingVtail) &&
           (systemSettings.airframeType < SystemSettings.AirframeType.GroundVehicleCar));
}

function isFixedWing() {
    return (systemSettings.airframeType <= SystemSettings.AirframeType.FixedWingVtail);
}

function isGroundVehicle() {
    return (systemSettings.airframeType >= SystemSettings.AirframeType.GroundVehicleCar);
}

function freeMemoryBytes() {
    return systemStats.heapRemaining;
}

function freeMemoryKBytes() {
    return (systemStats.heapRemaining / 1024).toFixed(2);
}

function freeMemory() {
    return (freeMemoryBytes() > 1024) ? freeMemoryKBytes() + "Kb" : freeMemoryBytes() + "bytes";
}

function cpuLoad() {
    return systemStats.cpuLoad;
}

function cpuTemp() {
    return systemStats.cpuTemp;
}

function flightTimeValue() {
    return Math.round(systemStats.flightTime / 1000)
}

/*
 * Sensors
 *
*/
function isAttitudeValid() {
    return (systemAlarms.alarmAttitude == SystemAlarms.Alarm.OK);
}

function isAttitudeNotInitialised() {
    return (systemAlarms.alarmAttitude == SystemAlarms.Alarm.Uninitialised);
}

function isGpsValid() {
    return (systemAlarms.alarmGPS == SystemAlarms.Alarm.OK);
}

function isGpsNotInitialised() {
    return (systemAlarms.alarmGPS == SystemAlarms.Alarm.Uninitialised);
}

function isGpsStatusFix3D() {
    return (gpsPositionSensor.status == GPSPositionSensor.Status.Fix3D);
}

function isOplmConnected() {
    return (opLinkStatus.linkState == OPLinkStatus.LinkState.Connected);
}

function magSourceName() {
    return [magState.source == MagState.Source.Aux ? ["GPSv9", "Flexi", "I2C", "DJI"][auxMagSettings.type] + " " : ""] +
           ["Invalid", "OnBoard", "ExtMag"][magState.source];
}

function gpsSensorType() {
    return ["Unknown", "NMEA", "UBX", "UBX7", "UBX8", "DJI"][gpsPositionSensor.sensorType]
}

function gpsNumSat() {
    return gpsPositionSensor.satellites;
}

function gpsSatsInView() {
    return gpsSatellites.satsInView;
}

function gpsHdopInfo() {
    return gpsPositionSensor.hdop.toFixed(2) + "/" + gpsPositionSensor.vdop.toFixed(2) + "/" + gpsPositionSensor.pdop.toFixed(2);
}

function gpsAltitude() {
    return gpsPositionSensor.altitude.toFixed(2);
}

function gpsStatus() {
    return ["NO GPS", "NO FIX", "2D", "3D"][gpsPositionSensor.status];
}

function fusionAlgorithm() {
    return ["None", "Basic (No Nav)", "CompMag", "Comp+Mag+GPS", "EKFIndoor", "GPSNav (INS13)"][revoSettings.fusionAlgorithm];
}

function receiverQuality() {
    return (receiverStatus.quality > 0) ? receiverStatus.quality + "%" : "?? %";
}

function oplmLinkState() {
    return ["Disabled", "Enabled", "Disconnected", "Connecting", "Connected"][opLinkStatus.linkState];
}

/*
 * Battery
 *
*/
function batteryModuleEnabled() {
    return (hwSettings.optionalModulesBattery == HwSettings.OptionalModules.Enabled);
}

function batteryNbCells() {
    return flightBatterySettings.nbCells;
}

function batteryVoltage() {
    return flightBatteryState.voltage.toFixed(2);
}

function batteryCurrent() {
    return flightBatteryState.current.toFixed(2);
}

function batteryConsumedEnergy() {
    return flightBatteryState.consumedEnergy.toFixed(0);
}

function estimatedFlightTimeValue() {
    return Math.round(flightBatteryState.estimatedFlightTime);
}

function batteryAlarmColor() {
    //     Uninitialised, Ok,  Warning, Critical, Error
    return ["#2c2929", "green", "orange", "red", "red"][systemAlarms.alarmBattery];
}

function estimatedTimeAlarmColor() {
    return ((estimatedFlightTimeValue() <= 120) && (estimatedFlightTimeValue() > 60)) ? "orange" :
           (estimatedFlightTimeValue() <= 60) ? "red" : batteryAlarmColor();
}

/*
 * Pathplan and Waypoints
 *
*/
function isPathPlanEnabled() {
    return (flightStatus.flightMode == FlightStatus.FlightMode.PathPlanner);
}

function isPathPlanValid() {
    return (systemAlarms.alarmPathPlan == SystemAlarms.Alarm.OK);
}

function isPathDesiredActive() {
    return ((pathDesired.endEast != 0.0) || (pathDesired.endNorth != 0.0) || (pathDesired.endingVelocity != 0.0))
}

function isTakeOffLocationValid() {
    return (takeOffLocation.status == TakeOffLocation.Status.Valid);
}

function pathModeDesired() {
    return ["GOTO ENDPOINT","FOLLOW VECTOR","CIRCLE RIGHT","CIRCLE LEFT","FIXED ATTITUDE",
            "SET ACCESSORY","DISARM ALARM","LAND","BRAKE","VELOCITY","AUTO TAKEOFF"][pathDesired.mode]
}

function velocityDesiredDown() {
    return velocityDesired.down;
}

function pathDesiredEndDown() {
    return pathDesired.endDown;
}

function pathDesiredEndingVelocity() {
    return pathDesired.endingVelocity;
}

function currentWaypointActive() {
    return (waypointActive.index + 1);
}

function waypointCount() {
    return pathPlan.waypointCount;
}

function homeHeading() {
    return 180 / 3.1415 * Math.atan2(takeOffLocation.east - positionState.east, takeOffLocation.north - positionState.north);
}

function waypointHeading() {
    return 180 / 3.1415 * Math.atan2(pathDesired.endEast - positionState.east, pathDesired.endNorth - positionState.north);
}

function homeDistance() {
    return Math.sqrt(Math.pow((takeOffLocation.east - positionState.east), 2) +
                     Math.pow((takeOffLocation.north - positionState.north), 2));
}

function waypointDistance() {
    return Math.sqrt(Math.pow((pathDesired.endEast - positionState.east), 2) +
                     Math.pow((pathDesired.endNorth - positionState.north), 2));
}

/*
 * FlightModes, Stabilization, Thrust modes
 *
*/
function isFlightModeManual() {
    return (flightStatus.flightMode == FlightStatus.FlightMode.Manual);
}

function isFlightModeAssisted() {
    return (flightStatus.flightMode > FlightStatus.FlightMode.Stabilized6);
}

function isVtolPathFollowerSettingsThrustAuto() {
    return (vtolPathFollowerSettings.thrustControl == VtolPathFollowerSettings.ThrustControl.Auto);
}

function flightModeName() {
    return ["MANUAL", "STAB 1", "STAB 2", "STAB 3", "STAB 4", "STAB 5", "STAB 6",
            "POS HOLD", "COURSELOCK", "VEL ROAM", "HOME LEASH", "ABS POS", "RTB",
            "LAND", "PATHPLAN", "POI", "AUTOCRUISE", "AUTOTAKEOFF", "AUTOTUNE"][flightStatus.flightMode];
}

function flightModeColor() {
    return ["gray", "green", "green", "green", "green", "green", "green",
            "cyan", "cyan", "cyan", "cyan", "cyan", "cyan",
            "cyan", "cyan", "cyan", "cyan", "cyan", "cyan"][flightStatus.flightMode];
}

function thrustMode() {
    return !isFlightModeAssisted() ? stabilizationDesired.stabilizationModeThrust :
           (isFlightModeAssisted() && isVtolOrMultirotor() && isVtolPathFollowerSettingsThrustAuto()) ?
            StabilizationDesired.StabilizationDesiredConstants.StabilizationModeCount : (isFlightModeAssisted() && isFixedWing()) ?
            StabilizationDesired.StabilizationDesiredConstants.StabilizationModeCount : 0;
}

function thrustModeName() {
    // Last "Auto" Thrust mode is added to UAVO enum list
    // Lower case modes are never displayed/used for Thrust
    return ["MANUAL", "rate", "ratetrainer", "attitude", "axislock", "weakleveling", "virtualbar", "acro+ ", "rattitude",
            "ALT HOLD", "ALT VARIO", "CRUISECTRL", "systemident", "AUTO"][thrustMode()]
}

function thrustModeColor() {
    return ["green", "grey", "grey", "grey", "grey", "grey", "grey", "grey", "grey",
            "green", "green", "green", "grey", "cyan"][thrustMode()];
}

function armStatusName() {
    return ["DISARMED","ARMING","ARMED"][flightStatus.armed];
}

function armStatusColor() {
    return ["gray", "orange", "green"][flightStatus.armed];
}

/*
 * Alarms
 *
*/
function autopilotStatusColor() {
    return statusColor(systemAlarms.alarmGuidance)
}

function rcInputStatusColor() {
    return statusColor(systemAlarms.alarmManualControl)
}

function statusColor(module) {
    return ["gray", "green", "red", "red", "red"][module];
}

function masterCautionWarning() {
    return ((systemAlarms.alarmBootFault > SystemAlarms.Alarm.OK) ||
           (systemAlarms.alarmOutOfMemory > SystemAlarms.Alarm.OK) ||
           (systemAlarms.alarmStackOverflow > SystemAlarms.Alarm.OK) ||
           (systemAlarms.alarmCPUOverload > SystemAlarms.Alarm.OK) ||
           (systemAlarms.alarmEventSystem > SystemAlarms.Alarm.OK))
}

