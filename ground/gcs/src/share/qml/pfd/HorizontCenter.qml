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

Item {
    id: sceneItem
    property variant sceneSize
    property real horizontCenter : world_center.y + world_center.height/2

    SvgElementImage {
        id: world_center
        elementName: "center-arrows"
        sceneSize: background.sceneSize

        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
    }

    SvgElementImage {
        id: world_center_plane
        elementName: "center-plane"
        sceneSize: background.sceneSize

        width: Math.floor(scaledBounds.width * sceneItem.width)
        height: Math.floor(scaledBounds.height * sceneItem.height)

        x: Math.floor(scaledBounds.x * sceneItem.width)
        y: Math.floor(scaledBounds.y * sceneItem.height)
    }
}
