import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Popup {
    id: control

    property string title: ""
    property string content: ""
    property string iconSource: "qrc:/resources/icons/info.svg"
    property string buttonText: "OK"
    property int titleFontPixelSize: 18
    property int contentFontPixelSize: 14
    property bool closeOnAccepted: true

    signal accepted()

    function show(messageTitle, messageContent) {
        if (messageTitle !== undefined) {
            title = messageTitle
        }
        if (messageContent !== undefined) {
            content = messageContent
        }
        open()
    }

    modal: true
    focus: true
    padding: 0
    closePolicy: Popup.CloseOnEscape
    width: Math.min(parent ? parent.width - 48 : 420, 420)
    height: Math.max(184, messageContent.y + messageContent.implicitHeight + 78)
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0

    Overlay.modal: Rectangle {
        color: "#6f768099"
    }

    background: Rectangle {
        radius: 5
        color: "#ffffff"
        border.color: "#e2e6ee"
        border.width: 1

        layer.enabled: true
    }

    contentItem: Item {
        implicitWidth: 420
        implicitHeight: 184

        Rectangle {
            id: iconFrame
            width: 50
            height: 50
            radius: 13
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 26
            color: "#f5f8ff"
            border.color: "#dbe6ff"
            border.width: 2

            IconImage {
                anchors.centerIn: parent
                width: 30
                height: 30
                sourceSize.width: 30
                sourceSize.height: 30
                source: control.iconSource
                color: "#2563eb"
            }
        }

        Text {
            id: titleLabel
            anchors.left: iconFrame.right
            anchors.leftMargin: 18
            anchors.right: closeButton.left
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 31
            text: control.title
            color: "#111827"
            font.pixelSize: control.titleFontPixelSize
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            id: messageContent
            anchors.left: titleLabel.left
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: titleLabel.bottom
            anchors.topMargin: 16
            text: control.content
            color: "#5f6878"
            font.pixelSize: control.contentFontPixelSize
            lineHeight: 1.35
            wrapMode: Text.WordWrap
        }

        Item {
            id: closeButton
            width: 30
            height: 30
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: parent.top
            anchors.topMargin: 24
            activeFocusOnTab: true

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Close")

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: closeMouseArea.pressed ? "#f5f7fb" : "#ffffff"
                border.color: closeMouseArea.containsMouse ? "#cfd6e3" : "#e1e6ef"
                border.width: 2
            }

            IconImage {
                anchors.centerIn: parent
                width: 13
                height: 13
                sourceSize.width: 13
                sourceSize.height: 13
                source: "qrc:/resources/icons/cross.svg"
                color: "#111827"
            }

            MouseArea {
                id: closeMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: control.close()
            }

            Keys.onReturnPressed: control.close()
            Keys.onSpacePressed: control.close()
        }

        StyledButton {
            id: okButton
            text: control.buttonText
            styleName: "primary-button"
            fontPixelSize: 12
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 22
            width: 82
            height: 30
            onClicked: {
                control.accepted()
                if (control.closeOnAccepted) {
                    control.close()
                }
            }
        }
    }
}
