import QtQuick 2.4

import "../uav.js" as UAV

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

        rotation: -UAV.attitudeYaw()
        transformOrigin: Item.Center
    }

    SvgElementImage {
        id: compass_home
        elementName: "compass-home" // Cyan point
        sceneSize: sceneItem.sceneSize
        smooth: true

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        rotation: -UAV.attitudeYaw() + UAV.homeHeading()
        transformOrigin: Item.Bottom

        visible: UAV.isTakeOffLocationValid()
    }

    SvgElementImage {
        id: compass_waypoint // Double Purple arrow
        elementName: "compass-waypoint"
        sceneSize: sceneItem.sceneSize
        smooth: true

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        rotation: -UAV.attitudeYaw() + UAV.waypointHeading()
        transformOrigin: Item.Center

        visible: (UAV.isFlightModeAssisted() && UAV.isPathDesiredActive())
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
            text: Math.floor(UAV.attitudeYaw()).toFixed()
            color: "white"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 1.2
            }
            anchors.centerIn: parent
        }
    }

}
