import QtQuick 2.4
import osgQtQuick 1.0
import PfdQmlEnums 1.0

OSGViewport {
    anchors.fill: parent
    focus: true
    logarithmicDepthBuffer: true
    sceneData: skyNode
    camera: camera

    OSGSkyNode {
        id: skyNode
        sceneData: terrainNode
        dateTime: getDateTime()
        minimumAmbientLight: qmlWidget.minimumAmbientLight

        function getDateTime() {
            switch(qmlWidget.timeMode) {
            case Pfd.Local:
                return new Date();
            case Pfd.PredefinedTime:
                return qmlWidget.dateTime;
            }
        }

    }

    OSGFileNode {
        id: terrainNode
        source: qmlWidget.terrainFile
        async: false
        optimizeMode: OSGFileNode.OptimizeAndCheck
    }

    OSGModelNode {
        id: modelNode
        modelData: modelGroup
        sceneData: terrainNode
        clampToTerrain: true

        attitude: uavAttitude()
        position: uavPosition()

        function uavAttitude() {
            return Qt.vector3d(AttitudeState.Pitch, AttitudeState.Roll, -AttitudeState.Yaw);
        }

        function uavPosition() {
            switch(qmlWidget.positionMode) {
            case Pfd.GPS:
                return uavGPSPosition();
            case Pfd.Home:
                return uavHomePosition();
            case Pfd.Predefined:
                return uavPredefinedPosition();
            }
        }

        function uavGPSPosition() {
            switch(GPSPositionSensor.Status) {
            case 0: // NoGPS
            case 1: // NoFix
                return uavDefaultPosition();
            case 2: // Fix2D
            case 3: // Fix3D
                return uavActualGPSPosition();
            default:
                return uavDefaultPosition();
            }
        }

        function uavActualGPSPosition() {
            return Qt.vector3d(
                GPSPositionSensor.Longitude / 10000000.0,
                GPSPositionSensor.Latitude / 10000000.0,
                GPSPositionSensor.Altitude);
        }

        function uavHomePosition() {
            return (HomeLocation.Set == 1) ? uavActualHomePosition() : uavDefaultPosition();
        }

        function uavActualHomePosition() {
            return Qt.vector3d(
                HomeLocation.Longitude / 10000000.0,
                HomeLocation.Latitude / 10000000.0,
                HomeLocation.Altitude);
        }

        function uavPredefinedPosition() {
            return Qt.vector3d(qmlWidget.longitued, qmlWidget.latitude, qmlWidget.altitude);
        }

        function uavDefaultPosition() {
            // https://www.google.com.au/maps/search/c+hotel+corfu/@39.6571126,19.8044632,356m/data=!3m1!1e3
            return Qt.vector3d(19.805158, 39.657380, 100);
        }

    }

    // this group is needed as the target for the camera
    // using modelTransformNode leads to strange camera behavior (the reason is not clear why)
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

    OSGFileNode {
        id: modelFileNode
        source: qmlWidget.modelFile
        async: false
        optimizeMode: OSGFileNode.OptimizeAndCheck
    }

    OSGCamera {
        id: camera
        fieldOfView: 90
        manipulatorMode: OSGCamera.Track
        // use model to compute camera home position
        node: modelGroup
        // model will be tracked
        trackNode: modelGroup
    }
}
