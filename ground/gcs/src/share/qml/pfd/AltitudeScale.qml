/*
 * Copyright (C) 2016 The LibrePilot Project
 * Contact: http://www.librepilot.org
 *
 * This file is part of LibrePilot GCS.
 *
 * LibrePilot GCS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LibrePilot GCS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LibrePilot GCS.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.4

import "../js/common.js" as Utils
import "../js/uav.js" as UAV

Item {
    id: sceneItem
    property variant sceneSize

    property real altitude : -pfdContext.altitudeFactor * UAV.positionStateDown()

    SvgElementImage {
        id: altitude_window
        elementName: "altitude-window"
        sceneSize: sceneItem.sceneSize
        clip: true

        visible: pfdContext.altitudeUnit != 0

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "altitude-window")

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        SvgElementImage {
            id: altitude_scale

            elementName: "altitude-scale"
            sceneSize: sceneItem.sceneSize

            anchors.verticalCenter: parent.verticalCenter
            // The altitude scale represents 10 units (ft or meters),
            // move using decimal term from value to display
            anchors.verticalCenterOffset: height/10 * (altitude - Math.floor(altitude))
            anchors.left: parent.left

            property int topNumber: Math.floor(altitude)+5

            // Altitude numbers
            Column {
                Repeater {
                    model: 10
                    Item {
                        height: altitude_scale.height / 10
                        width: altitude_window.width

                        Text {
                            text: altitude_scale.topNumber - index
                            color: "white"
                            font.pixelSize: parent.height / 3
                            font.family: pt_bold.name

                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.top
                        }
                    }
                }
            }
        }

        SvgElementImage {
            id: altitude_vector
            elementName: "altitude-vector"
            sceneSize: sceneItem.sceneSize

            height: -UAV.nedAccelDown() * altitude_scale.height / 10

            anchors.left: parent.left
            anchors.bottom: parent.verticalCenter
        }

        SvgElementImage {
            id: altitude_waypoint
            elementName: "altitude-waypoint"
            sceneSize: sceneItem.sceneSize
            visible: (UAV.isFlightModeAssisted() && (UAV.pathDesiredEndDown() != 0.0))

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            anchors.verticalCenterOffset: -altitude_scale.height / 10 * 
                                          (UAV.positionStateDown() - UAV.pathDesiredEndDown()) * pfdContext.altitudeFactor
        }
    }

    SvgElementImage {
        id: altitude_box
        clip: true

        visible: pfdContext.altitudeUnit != 0

        elementName: "altitude-box"
        sceneSize: sceneItem.sceneSize

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd/pfd.svg", "altitude-box")

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        Text {
            id: altitude_text
            text: "  " + altitude.toFixed(1)
            color: "white"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 0.35
                weight: Font.DemiBold
            }
            anchors.centerIn: parent
        }
    }

    SvgElementImage {
        id: altitude_unit_box
        elementName: "altitude-unit-box"
        sceneSize: sceneItem.sceneSize

        visible: pfdContext.altitudeUnit != 0

        anchors.top: altitude_window.bottom
        anchors.right: altitude_window.right
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        Text {
            id: altitude_unit_text
            text: pfdContext.altitudeUnit
            color: "cyan"
            font {
                family: pt_bold.name
                pixelSize: parent.height * 0.6
                weight: Font.DemiBold
            }
            anchors.centerIn: parent
        }
    }
}
