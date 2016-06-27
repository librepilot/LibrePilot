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
import QtQuick.Controls 1.4

import Pfd 1.0
import OsgQtQuick 1.0

import "js/common.js" as Utils

Item {
    OSGViewport {
        id: osgViewport

        anchors.fill: parent
        focus: true

        sceneNode: skyNode
        camera: camera
        manipulator: earthManipulator
        incrementalCompile: true

        OSGCamera {
            id: camera
            fieldOfView: 90
        }

        OSGEarthManipulator {
            id: earthManipulator
        }

        OSGSkyNode {
            id: skyNode
            sceneNode: terrainNode
            viewport: osgViewport
            dateTime: Utils.getDateTime()
            minimumAmbientLight: pfdContext.minimumAmbientLight
        }

        OSGFileNode {
            id: terrainNode
            source: pfdContext.terrainFile
            async: false
        }

    }

    BusyIndicator {
        width: 24
        height: 24
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 4

        running: osgViewport.busy
    }
}
