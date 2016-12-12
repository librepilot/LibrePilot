/**
 ******************************************************************************
 *
 * @file       aboutdialog.qml
 * @author     The LibrePilot Team http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 1.2

Rectangle {
    id: container
    width: 600
    height: 480

    property AuthorsModel authors: AuthorsModel {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        RowLayout {
            opacity: 1
            Image {
                id: logo
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: parent.top
                anchors.topMargin: 10
                source: "../images/librepilot_logo_128.png"
                z: 100
                fillMode: Image.PreserveAspectFit
                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        dialog.openUrl("http://www.librepilot.org")
                    }
                }
            }

            Rectangle {
                anchors.left: logo.right
                anchors.margins: 10
                anchors.right: parent.right
                anchors.top: parent.top
                Layout.fillHeight: true
                Layout.fillWidth: true
                anchors.bottom: parent.bottom
                color: "transparent"

                ColumnLayout {
                    anchors.fill: parent
                    Text {
                        id: headerLabel
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        font.bold: true
                        text: qsTr("LibrePilot Ground Control Station")
                    }
                    Text {
                        id: versionLabel
                        Layout.fillWidth: true
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        text: version
                    }
                    TabView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Tab {
                            title: qsTr("Contributors")
                            anchors.leftMargin: 6
                            ScrollView {
                                frameVisible: false
                                ListView {
                                    id: authorsView
                                    anchors.fill: parent
                                    spacing: 3
                                    model: authors
                                    delegate: Text {
                                        font.pixelSize: 12
                                        text: name
                                    }
                                    clip: true
                                }
                            }
                        }
                        Tab {
                            title: qsTr("Credits")
                            // margin hack to fix broken UI when frameVisible is false
                            anchors.margins: 1
                            anchors.rightMargin: 0
                            TextArea {
                                readOnly: true
                                frameVisible: false
                                wrapMode: TextEdit.WordWrap
                                textFormat: TextEdit.RichText
                                text: credits
                                onLinkActivated: Qt.openUrlExternally(link)
                            }
                        }
                        Tab {
                            title: qsTr("License")
                            // margin hack to fix broken UI when frameVisible is false
                            anchors.margins: 1
                            anchors.rightMargin: 0
                            TextArea {
                                readOnly: true
                                frameVisible: false
                                wrapMode: TextEdit.WordWrap
                                textFormat: TextEdit.RichText
                                text: license
                                onLinkActivated: Qt.openUrlExternally(link)
                            }
                        }
                    }
                    Button {
                        id: button
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        text: qsTr("Ok")
                        activeFocusOnPress: true
                        onClicked: dialog.close()
                    }
                }
            }
        }
    }
}
