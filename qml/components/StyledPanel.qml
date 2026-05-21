import QtQuick

Rectangle {
    id: panel

    property string title: ""
    property int titleFontPixelSize: 11
    property int headerHeight: 29
    default property alias content: body.data

    implicitHeight: 220
    color: "#ffffff"
    border.color: "#d6dbe2"
    border.width: 1
    radius: 0
    clip: true

    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: panel.headerHeight
        color: "#f3f5f8"
        radius: panel.radius

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: panel.radius
            color: parent.color
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#dfe3e8"
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: panel.title
            color: "#23272f"
            font.pixelSize: panel.titleFontPixelSize
            font.bold: true
        }
    }

    Item {
        id: body
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
