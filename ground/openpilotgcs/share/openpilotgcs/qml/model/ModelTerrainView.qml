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

        attitude: Qt.vector3d(AttitudeState.Pitch, AttitudeState.Roll, -AttitudeState.Yaw)
        position: Qt.vector3d(lon(), lat(), alt())

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
