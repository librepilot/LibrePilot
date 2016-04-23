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

Image {
    id: sceneItem
    property variant sceneSize
    property string elementName
    property string svgFileName: "pfd/pfd.svg"
    property int vSlice: 0
    property int vSliceCount: 0
    property int hSlice: 0
    property int hSliceCount: 0
    //border property is useful to extent the area of image e bit,
    //so it looks well antialiased when rotated
    property int border: 0
    property variant scaledBounds: svgRenderer.scaledElementBounds(svgFileName, elementName)

    sourceSize.width: Math.round(sceneSize.width*scaledBounds.width)
    sourceSize.height: Math.round(sceneSize.height*scaledBounds.height)

    x: Math.floor(scaledBounds.x * sceneSize.width)
    y: Math.floor(scaledBounds.y * sceneSize.height)

    Component.onCompleted: reloadImage()
    onElementNameChanged: reloadImage()
    onSceneSizeChanged: reloadImage()

    function reloadImage() {
        var params = ""
        if (hSliceCount > 1)
            params += "hslice="+hSlice+":"+hSliceCount+";"
        if (vSliceCount > 1)
            params += "vslice="+vSlice+":"+vSliceCount+";"
        if (border > 0)
            params += "border="+border+";"

        if (params != "")
            params = "?" + params

        source = "image://svg/"+svgFileName+"!"+elementName+params
        scaledBounds = svgRenderer.scaledElementBounds(svgFileName, elementName)
    }
}
