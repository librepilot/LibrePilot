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

import "../js/uav.js" as UAV

Item {
    id: sceneItem
    property variant sceneSize
    property real groundSpeed : pfdContext.speedFactor * UAV.currentVelocity()

    SvgElementImage {
        id: speed_window
        elementName: "speed-window"
        sceneSize: sceneItem.sceneSize
        clip: true

        visible: pfdContext.speedUnit != 0

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)

        SvgElementImage {
            id: speed_scale

            elementName: "speed-scale"
            sceneSize: sceneItem.sceneSize

            anchors.verticalCenter: parent.verticalCenter
            // The speed scale represents 10 units,
            // move using decimal term from value to display
            anchors.verticalCenterOffset: height/10 * (sceneItem.groundSpeed-Math.floor(sceneItem.groundSpeed))
            anchors.right: parent.right

            property int topNumber: Math.floor(sceneItem.groundSpeed)+5

            // speed numbers
            Column {
                width: speed_window.width
                anchors.right: speed_scale.right

                Repeater {
                    model: 10
                    Item {
                        height: speed_scale.height / 10
                        width: speed_window.width

                        Text {
                            //don't show negative numbers
                            text: speed_scale.topNumber - index
                            color: "white"
                            visible: speed_scale.topNumber - index >= 0

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
            id: speed_waypoint
            elementName: "speed-waypoint"
            sceneSize: sceneItem.sceneSize
            visible: (UAV.isFlightModeAssisted() && UAV.isPathDesiredActive())

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            anchors.verticalCenterOffset: speed_scale.height / 10 * (sceneItem.groundSpeed - (UAV.pathDesiredEndingVelocity() * pfdContext.speedFactor))
        }
    }

    SvgElementImage {
        id: speed_box
        clip: true
        elementName: "speed-box"
        sceneSize: sceneItem.sceneSize

        visible: pfdContext.speedUnit != 0

        x: scaledBounds.x * sceneItem.width
        y: scaledBounds.y * sceneItem.height
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        Text {
            id: speed_text
            text: sceneItem.groundSpeed.toFixed(1)+"  "
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
        id: speed_unit_box
        elementName: "speed-unit-box"
        sceneSize: sceneItem.sceneSize

        visible: pfdContext.speedUnit != 0

        anchors.top: speed_window.bottom
        anchors.right: speed_window.right
        width: scaledBounds.width * sceneItem.width
        height: scaledBounds.height * sceneItem.height

        Text {
           id: speed_unit_text
           text: pfdContext.speedUnit
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
