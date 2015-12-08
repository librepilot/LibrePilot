import QtQuick 2.4

import UAVTalk.AttitudeState 1.0
import UAVTalk.PositionState 1.0
import UAVTalk.PathDesired 1.0
import UAVTalk.TakeOffLocation 1.0

Item {
    id: sceneItem
    property variant sceneSize

    SvgElementImage {
        id: compass_fixed
        elementName: "compass-fixed"
        sceneSize: sceneItem.sceneSize

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
    }

    SvgElementImage {
        id: compass_plane
        elementName: "compass-plane"
        sceneSize: sceneItem.sceneSize

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
    }

    SvgElementImage {
        id: compass_wheel
        elementName: "compass-wheel"
        sceneSize: sceneItem.sceneSize
        smooth: true

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        rotation: -attitudeState.yaw
        transformOrigin: Item.Center
    }

    SvgElementImage {
        id: compass_home
        elementName: "compass-home" // Cyan point
        sceneSize: sceneItem.sceneSize
        smooth: true

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        property real home_degrees: 180 / 3.1415 * Math.atan2(takeOffLocation.east - positionState.east, takeOffLocation.north - positionState.north)

        rotation: -attitudeState.yaw + home_degrees
        transformOrigin: Item.Bottom

        visible: (takeOffLocation.status == Status.Valid)
    }

    SvgElementImage {
        id: compass_waypoint // Double Purple arrow
        elementName: "compass-waypoint"
        sceneSize: sceneItem.sceneSize
        smooth: true

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        property real course_degrees: 180 / 3.1415 * Math.atan2(pathDesired.endEast - positionState.east, pathDesired.endNorth - positionState.north)

        rotation: -attitudeState.yaw + course_degrees
        transformOrigin: Item.Center

        visible: ((pathDesired.endEast != 0.0) && (pathDesired.endNorth != 0.0))
    }

    Item {
        id: compass_text_box

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "compass-text")

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        Text {
            id: compass_text
            text: Math.floor(attitudeState.yaw).toFixed()
            color: "white"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.2
            }
            anchors.centerIn: parent
        }
    }

}
