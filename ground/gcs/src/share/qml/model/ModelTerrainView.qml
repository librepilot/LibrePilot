import QtQuick 2.4
import Pfd 1.0
import OsgQtQuick 1.0

import UAVTalk.AttitudeState 1.0
import UAVTalk.HomeLocation 1.0
import UAVTalk.GPSPositionSensor 1.0

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
            return Qt.vector3d(attitudeState.pitch, attitudeState.roll, -attitudeState.yaw);
        }

        function uavPosition() {
            switch(gpsPositionSensor.status) {
            case Status.Fix2D:
            case Status.Fix3D:
                return uavGPSPosition();
            case Status.NoFix:
            case Status.NoGPS:
            default:
                return (homeLocation.set == Set.True) ? uavHomePosition() : uavDefaultPosition();
            }
        }

        function uavGPSPosition() {
            return Qt.vector3d(
                gpsPositionSensor.longitude / 10000000.0,
                gpsPositionSensor.latitude / 10000000.0,
                gpsPositionSensor.altitude);
        }

        function uavHomePosition() {
            return Qt.vector3d(
                homeLocation.longitude / 10000000.0,
                homeLocation.latitude / 10000000.0,
                homeLocation.altitude);
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
