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
