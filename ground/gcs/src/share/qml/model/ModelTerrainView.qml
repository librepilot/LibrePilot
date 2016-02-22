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

import Pfd 1.0
import OsgQtQuick 1.0

import "../common.js" as Utils
import "../uav.js" as UAV

OSGViewport {
    anchors.fill: parent
    focus: true
    sceneData: skyNode
    camera: camera

    OSGSkyNode {
        id: skyNode
        sceneData: sceneGroup
        dateTime: Utils.getDateTime()
        minimumAmbientLight: qmlWidget.minimumAmbientLight
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

        attitude: UAV.attitude()
        position: UAV.position()
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
