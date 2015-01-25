import QtQuick 2.0
import osgQtQuick 1.0
import PfdQmlEnums 1.0

OSGViewport {
    id: fullview
    anchors.fill: parent
    focus: true
    //color: "transparent"
    //opacity: 0.999
    updateMode: OSGViewport.Discrete
    logarithmicDepthBuffer: true
    camera: camera
    sceneData: skyNode

    OSGSkyNode {
        id: skyNode
        sceneData: terrainNode
        dateTime: new Date();
    }

    OSGNodeFile {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
    }

    OSGModelNode {
        id: modelNode
        modelData: modelGroup
        sceneData: terrainNode

        yaw: AttitudeState.Yaw
        pitch: AttitudeState.Pitch
        roll: AttitudeState.Roll

        latitude: lat()
        longitude: lon()
        altitude: alt()

        clampToTerrain: true

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
                return GPSPositionSensor.Altitude;
            case Pfd.Home:
                return HomeLocation.Altitude;
            case Pfd.Predefined:
                return qmlWidget.altitude;
            }
        }
    }

    OSGGroup {
        id: modelGroup
        children: [
            modelTransformNode
        ]
    }

    OSGTransformNode {
        id: modelTransformNode
        modelData: modelFileNode
        // model dimensions are in mm, scale to meters
        scale: Qt.vector3d(0.001, 0.001, 0.001)
    }

    OSGNodeFile {
        id: modelFileNode
        source: qmlWidget.modelFile
        async: false
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        manipulatorMode: OSGCamera.Track
        trackNode: modelGroup
    }
}
