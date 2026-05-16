import QtQuick 2.15
import QtQuick.Controls.Basic

Item {
    id: udpConnection

    property alias host: hostInput.text
    property alias remotePort: remotePortInput.text
    property alias localListenPort: localListenPortInput.text

    width: 305
    height: 34

    Row {
        id: controlRow
        anchors.fill: parent
        spacing: 8

        TextField {
            id: hostInput
            width: 145
            height: parent.height
            text: "192.168.1.10"
            color: "white"
            selectedTextColor: "white"
            selectionColor: "#4c8bf5"
            font.pixelSize: 14
            leftPadding: 12
            rightPadding: 12
            verticalAlignment: TextInput.AlignVCenter

            background: Rectangle {
                radius: 6
                color: "#1e1e1e"
                border.color: hostInput.hovered || hostInput.activeFocus ? "#4c8bf5" : "#424242"
                border.width: 1
            }
        }

        Text {
            width: 8
            height: parent.height
            text: ":"
            color: "#d6d6d6"
            font.pixelSize: 15
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        TextField {
            id: remotePortInput
            width: 96
            height: parent.height
            text: "1234"
            color: "white"
            selectedTextColor: "white"
            selectionColor: "#4c8bf5"
            font.pixelSize: 14
            leftPadding: 12
            rightPadding: 12
            validator: IntValidator { bottom: 1; top: 65535 }
            verticalAlignment: TextInput.AlignVCenter

            background: Rectangle {
                radius: 6
                color: "#1e1e1e"
                border.color: remotePortInput.hovered || remotePortInput.activeFocus ? "#4c8bf5" : "#424242"
                border.width: 1
            }
        }

        Rectangle {
            id: settingsButton
            width: 34
            height: parent.height
            radius: 6
            color: settingsArea.pressed || settingsPopup.opened ? "#303030" : "#1e1e1e"
            border.color: settingsArea.containsMouse || settingsPopup.opened ? "#4c8bf5" : "#424242"
            border.width: 1

            Canvas {
                anchors.centerIn: parent
                width: 11
                height: 8

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "white"
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(1, 1)
                    ctx.lineTo(width / 2, height - 1)
                    ctx.lineTo(width - 1, 1)
                    ctx.stroke()
                }
            }

            MouseArea {
                id: settingsArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: settingsPopup.opened ? settingsPopup.close() : settingsPopup.open()
            }
        }
    }

    Popup {
        id: settingsPopup
        x: settingsButton.x + settingsButton.width / 2 - width / 2
        y: settingsButton.y + settingsButton.height + 8
        width: 270
        height: 90
        padding: 0
        modal: false
        focus: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Item {}

        contentItem: Item {
            Rectangle {
                anchors.fill: panel
                anchors.topMargin: 3
                radius: panel.radius
                color: "#000000"
                opacity: 0.35
            }

            Rectangle {
                width: 14
                height: 14
                x: settingsPopup.width / 2 - width / 2
                y: 0
                rotation: 45
                color: "#202020"
                border.color: "#353535"
                border.width: 1
            }

            Rectangle {
                id: panel
                x: 0
                y: 7
                width: parent.width
                height: parent.height - 7
                radius: 8
                color: "#202020"
                border.color: "#353535"
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.topMargin: 13
                    anchors.bottomMargin: 13
                    spacing: 7

                    Row {
                        width: parent.width
                        height: 28
                        spacing: 12

                        Text {
                            width: 124
                            height: parent.height
                            text: "Local Listen Port"
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                        }

                        TextField {
                            id: localListenPortInput
                            width: parent.width - 124 - parent.spacing
                            height: parent.height
                            text: "1234"
                            color: "white"
                            selectedTextColor: "white"
                            selectionColor: "#4c8bf5"
                            font.pixelSize: 13
                            leftPadding: 10
                            rightPadding: 10
                            validator: IntValidator { bottom: 1; top: 65535 }
                            verticalAlignment: TextInput.AlignVCenter

                            background: Rectangle {
                                radius: 5
                                color: "#222222"
                                border.color: localListenPortInput.hovered || localListenPortInput.activeFocus ? "#4c8bf5" : "#484848"
                                border.width: 1
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        text: "Port on this computer to receive data."
                        color: "#a9a9a9"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
