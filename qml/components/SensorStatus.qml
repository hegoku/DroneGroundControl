import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: sensor
    property bool on: false
    property string label: ""
    property string onImg: ""
    property string offImg: ""
    property string onColor: "#F6C84F"
    property string offColor: "#353535"

    width: 100
    height: 100

    Rectangle {
        id: bg
        anchors.fill: parent
        anchors.margins: Math.max(4, parent.height * 0.08)
        radius: Math.max(8, parent.height * 0.08)
        color: "#1e1e1e"

        // subtle shadow
        Rectangle {
            anchors.fill: parent
            radius: Math.max(8, parent.height * 0.08)
            color: sensor.on ? onColor : offColor
            opacity: sensor.on ? 0.7 : 0.2
            anchors.margins: -1
            z: -1
        }

        Image {
            id: icon
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            source: sensor.on ? sensor.onImg : sensor.offImg
            width: parent.width*0.8
            height: parent.height*0.8
            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: text
            anchors.top: icon.bottom
            anchors.bottom: bg.bottom
            anchors.bottomMargin: bg.height * 0.06
            anchors.horizontalCenter: parent.horizontalCenter
            text: sensor.label
            font.pixelSize: height*1.5
            width: parent.width
            height: parent.height*0.2
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: sensor.on ? onColor : offColor
            elide: Text.ElideRight
        }
    }
}
