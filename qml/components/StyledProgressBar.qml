import QtQuick
import QtQuick.Controls.Basic

ProgressBar {
    id: control

    property color progressColor: "#4c75f2"
    property color trackColor: "#e4e8ee"
    property color disabledProgressColor: "#c6ccd5"
    property color disabledTrackColor: "#eef1f5"

    implicitWidth: 150
    implicitHeight: 6

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: height / 2
        color: control.enabled ? control.trackColor : control.disabledTrackColor
    }

    contentItem: Item {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: control.enabled ? control.progressColor : control.disabledProgressColor
        }
    }
}
