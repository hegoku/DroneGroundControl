import QtQuick

Rectangle {
    id: panel

    property string title: ""
    default property alias content: body.data

    implicitHeight: 220
    color: "#ffffff"
    border.color: "#d6dbe2"
    border.width: 1
    radius: 6

    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: "#f3f5f8"
        border.color: "#dfe3e8"
        border.width: 1
        radius: panel.radius

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: panel.radius
            color: parent.color
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: panel.title
            color: "#23272f"
            font.pixelSize: 16
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
