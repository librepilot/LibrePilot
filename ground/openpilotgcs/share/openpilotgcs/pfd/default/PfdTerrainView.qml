import QtQuick 2.0
import PfdQmlEnums 1.0
import osgQtQuick 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    sceneData: skyNode
    camera: camera
    //color: "transparent"
    //opacity: 0.999
    mode: OSGViewport.Buffer
    focus: true

    OSGSkyNode {
        id: skyNode
        sceneData: terrainNode
    }

    OSGNodeFile {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        manipulatorMode: OSGCamera.User

        yaw: AttitudeState.Yaw
        pitch: AttitudeState.Pitch
        roll: AttitudeState.Roll

        latitude: lat()
        longitude: lon()
        altitude: alt()

        function lat() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Latitude / 10000000.0;
            case Pfd.Home:
                return HomeLocation.Latitude / 10000000.0;
            case Pfd.Predefined:
                return qmlWidget.latitude;
            }
        }

        function lon() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Longitude / 10000000.0;
            case Pfd.Home:
                return HomeLocation.Longitude / 10000000.0;
            case Pfd.Predefined:
                return qmlWidget.longitude;
            }
        }

        function alt() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return GPSPositionSensor.Altitude / 10000000.0;
            case Pfd.Home:
                return HomeLocation.Altitude / 10000000.0;
            case Pfd.Predefined:
                return qmlWidget.altitude;
            }
        }

    }
}
