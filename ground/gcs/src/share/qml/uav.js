// ***********************
// common.js
//
// Common javascript uav utils
//
// Librepilot
// ***********************

Qt.include("common.js")

function attitude() {
    return Qt.vector3d(attitudeState.pitch, attitudeState.roll, -attitudeState.yaw);
}

function position() {
    switch(gpsPositionSensor.status) {
    case Status.Fix2D:
    case Status.Fix3D:
        return gpsPosition();
    case Status.NoFix:
    case Status.NoGPS:
    default:
        return (homeLocation.set == Set.True) ? homePosition() : defaultPosition();
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
    return Qt.vector3d(qmlWidget.longitude, qmlWidget.latitude, qmlWidget.altitude);
}
