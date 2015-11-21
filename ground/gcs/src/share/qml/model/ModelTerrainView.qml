import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

OSGViewport {
    anchors.fill: parent
    focus: true
    sceneData: skyNode
    camera: camera

    OSGSkyNode {
        id: skyNode
        sceneData: sceneGroup
        dateTime: getDateTime()
        minimumAmbientLight: qmlWidget.minimumAmbientLight

        function getDateTime() {
            switch(qmlWidget.timeMode) {
            case TimeMode.Local:
                return new Date();
            case TimeMode.Predefined:
                return qmlWidget.dateTime;
            }
        }

    }

    OSGGroup {
        id: sceneGroup
        children: [ terrainNode, modelNode ]
    }

    OSGFileNode {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
    }

    OSGModelNode {
        id: modelNode
        clampToTerrain: true
        modelData: modelTransformNode
        sceneData: terrainNode

        attitude: uavAttitude()
        position: uavPosition()

        function uavAttitude() {
            return Qt.vector3d(AttitudeState.Pitch, AttitudeState.Roll, -AttitudeState.Yaw);
        }

        function uavPosition() {
            switch(GPSPositionSensor.Status) {
            case 2: // Fix2D
            case 3: // Fix3D
                return uavGPSPosition();
            case 0: // NoGPS
            case 1: // NoFix
            default:
                return (HomeLocation.Set == 1) ? uavHomePosition() : uavDefaultPosition();
            }
        }

        function uavGPSPosition() {
            return Qt.vector3d(
                GPSPositionSensor.Longitude / 10000000.0,
                GPSPositionSensor.Latitude / 10000000.0,
                GPSPositionSensor.Altitude);
        }

        function uavHomePosition() {
            return Qt.vector3d(
                HomeLocation.Longitude / 10000000.0,
                HomeLocation.Latitude / 10000000.0,
                HomeLocation.Altitude);
        }

        function uavDefaultPosition() {
            return Qt.vector3d(qmlWidget.longitude, qmlWidget.latitude, qmlWidget.altitude);
        }

    }

    OSGTransformNode {
        id: modelTransformNode
        // model dimensions are in mm, scale to meters
        scale: Qt.vector3d(0.001, 0.001, 0.001)
        modelData: modelFileNode
    }

    OSGFileNode {
        id: modelFileNode
        source: qmlWidget.modelFile
        async: false
        optimizeMode: OptimizeMode.OptimizeAndCheck
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        logarithmicDepthBuffer: true
        manipulatorMode: ManipulatorMode.Track
        // use model to compute camera home position
        node: modelTransformNode
        // model will be tracked
        trackNode: modelTransformNode
    }
}
