import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: connectionButton
    property bool on: false
    property string onColor: "#F6C84F"
    property string offColor: "red"
    signal clicked()

    width: 100
    height: 100

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.margins: Math.max(4, parent.height * 0.1)
        color: on ? onColor : offColor
        radius: 180

        // subtle shadow
        Rectangle {
            anchors.fill: parent
            radius: 180
            color: '#3a3a3a'
            // opacity: on ? 0.7 : 0.2
            anchors.margins: -2
            z: -1
        }

        Image {
            id: icon
            // anchors.top: parent.top
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/resources/icons/plugin.svg"
            width: parent.width * 0.6
            height: parent.height * 0.6
            fillMode: Image.PreserveAspectFit
        }

        // Text {
        //     id: text
        //     anchors.top: icon.bottom
        //     anchors.bottom: bg.bottom
        //     anchors.bottomMargin: bg.height * 0.06
        //     anchors.horizontalCenter: parent.horizontalCenter
        //     text: on ? "Connected" : "Disconnected"
        //     font.pixelSize: height
        //     width: parent.width
        //     height: parent.height*0.2
        //     horizontalAlignment: Text.AlignHCenter
        //     verticalAlignment: Text.AlignVCenter
        //     color: on ? onColor : offColor
        //     elide: Text.ElideRight
        // }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: connectionButton.clicked()
        cursorShape: Qt.PointingHandCursor
    }

}
