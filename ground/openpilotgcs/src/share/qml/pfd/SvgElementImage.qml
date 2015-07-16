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
