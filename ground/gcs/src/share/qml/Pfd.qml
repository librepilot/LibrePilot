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

import "pfd" as Pfd

Rectangle {
    color: "#515151"

    Pfd.SvgElementImage {
        id: background
        elementName: "pfd-window"
        fillMode: Image.PreserveAspectFit
        anchors.fill: parent
        sceneSize: Qt.size(width, height)

        Rectangle {
            width: Math.floor(parent.paintedHeight * 1.319)
            height: Math.floor(parent.paintedHeight - parent.paintedHeight * 0.008)

            color: "transparent"
            border.color: "white"
            border.width: Math.floor(parent.paintedHeight * 0.008)
            radius: Math.floor(parent.paintedHeight * 0.01)
            anchors.centerIn: parent
        }

        Item {
            id: sceneItem

            FontLoader {
                id: pt_bold
                source: "qrc:/utils/fonts/PTS75F.ttf"
            }

            width: Math.floor((parent.paintedHeight * 1.32) - (parent.paintedHeight * 0.013))
            height: Math.floor(parent.paintedHeight - parent.paintedHeight * 0.02)
            property variant viewportSize : Qt.size(width, height)

            anchors.centerIn: parent
            clip: true

            Loader {
                id: worldLoader
                anchors.fill: parent
                focus: true
                source: pfdContext.terrainEnabled ? "pfd/PfdTerrainView.qml" : "pfd/PfdWorldView.qml"
            }

            Pfd.HorizontCenter {
                id: horizontCenterItem
                sceneSize: sceneItem.viewportSize
                anchors.fill: parent
            }

            Pfd.RollScale {
                id: rollscale
                sceneSize: sceneItem.viewportSize
                horizontCenter: horizontCenterItem.horizontCenter
                anchors.fill: parent
            }

            Pfd.SvgElementImage {
                id: side_slip_fixed
                elementName: "sideslip-fixed"
                sceneSize: sceneItem.viewportSize

                x: scaledBounds.x * sceneItem.width
            }

            Pfd.Compass {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }

            Pfd.SpeedScale {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }

            Pfd.AltitudeScale {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }

            Pfd.VsiScale {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
                visible: pfdContext.altitudeUnit != 0
            }

            Pfd.Info {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }

            Pfd.Panels {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }

            Pfd.Warnings {
                anchors.fill: parent
                sceneSize: sceneItem.viewportSize
            }
        }
    }
}
