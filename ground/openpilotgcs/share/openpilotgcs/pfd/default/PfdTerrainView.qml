import QtQuick 2.0
import osgQtQuick 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    sceneData: terrainNode
    //color: "transparent"
    //opacity: 0.999
    mode: OSGViewport.Buffer
    focus: true

    OSGEarthNode {
        id: terrainNode
        source: qmlWidget.earthFile
    }

    OSGCamera {
        id: camera

        fieldOfView: 90

        yaw: AttitudeState.Yaw
        pitch: AttitudeState.Pitch
        roll: AttitudeState.Roll

        latitude: qmlWidget.actualPositionUsed ?
            GPSPositionSensor.Latitude / 10000000.0 : qmlWidget.latitude
        longitude: qmlWidget.actualPositionUsed ?
            GPSPositionSensor.Longitude / 10000000.0 : qmlWidget.longitude
        altitude: qmlWidget.actualPositionUsed ?
            GPSPositionSensor.Altitude : qmlWidget.altitude
    }
}
