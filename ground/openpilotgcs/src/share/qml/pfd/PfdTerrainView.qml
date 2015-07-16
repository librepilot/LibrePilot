import QtQuick 2.4
import osgQtQuick 1.0
import PfdQmlEnums 1.0

OSGViewport {
    id: fullview
    // anchors.fill: parent
    focus: true
    sceneData: skyNode
    camera: camera

    property real horizontCenter : horizontCenterItem.horizontCenter

    // Factor for OSGview vertical offset
    property double factor: 0.04

    // Stretch height and apply offset
    ///height: height * (1 + factor)
    y: -height * factor

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
    }

    OSGCamera {
        id: camera
        fieldOfView: 100
        sceneData: terrainNode
        logarithmicDepthBuffer: true
        clampToTerrain: true
        manipulatorMode: OSGCamera.User

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

    Rectangle {
        // using rectange instead of svg rendered to pixmap
        // as it's much more memory efficient
        id: world
        smooth: true
        opacity: 0

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "horizon")
        width: Math.round(sceneItem.width*scaledBounds.width/2)*2
        height: Math.round(sceneItem.height*scaledBounds.height/2)*3

        property double pitch1DegScaledHeight: (svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-90").y -
                                                svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch90").y)/180.0

        property double pitch1DegHeight: sceneItem.height*pitch1DegScaledHeight


        transform: [
            Translate {
                id: pitchTranslate
                x: Math.round((world.parent.width - world.width)/2)
                // y is centered around world_center element
                y: Math.round(horizontCenter - world.height/2 +
                              AttitudeState.Pitch*world.pitch1DegHeight)
            },
            Rotation {
                angle: -AttitudeState.Roll
                origin.x : world.parent.width/2
                origin.y : horizontCenter
            }
        ]

    }

    Item {
        id: pitch_window
        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "pitch-window-terrain")

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height) - fullview.y
        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        rotation: -AttitudeState.Roll
        transformOrigin: Item.Center

        smooth: true
        clip: true

        SvgElementImage {
            id: pitch_scale
            elementName: "pitch-scale"

            //worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent
            //see comment for world transform
            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 64 //sometimes numbers are excluded from bounding rect

            smooth: true
        }

        SvgElementImage {
            id: horizont_line
            elementName: "center-line"

            opacity: 0.5

            //worldView is loaded with Loader, so background element is visible
            sceneSize: background.sceneSize
            anchors.centerIn: parent

            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 1
            smooth: true
       }

       SvgElementImage {
            id: pitch_0
            elementName: "pitch0"

            sceneSize: background.sceneSize
            anchors.centerIn: parent
            anchors.verticalCenterOffset: AttitudeState.Pitch*world.pitch1DegHeight
            border: 1
            smooth: true
       }
    }
}
