import QtQuick
import QtQuick.Controls

Control {
    id: control

    property string styleName: "cancel-button"
    property string text: ""
    signal clicked()

    implicitWidth: 96
    implicitHeight: 36
    padding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    opacity: enabled ? 1.0 : 0.55

    readonly property bool primary: styleName === "primary-button"
    readonly property color fillColor: primary
                                      ? (tapHandler.pressed ? "#3f67dc" : "#4c75f2")
                                      : (tapHandler.pressed ? "#f4f6f8" : "#ffffff")
    readonly property color strokeColor: primary ? "#4c75f2"
                                                  : (hoverHandler.hovered ? "#bfc7d1" : "#cfd5dd")
    readonly property color labelColor: primary ? "#ffffff" : "#2d333b"

    background: Rectangle {
        radius: 4
        color: control.enabled ? control.fillColor : (control.primary ? "#9fb5f8" : "#fbfcfd")
        border.color: control.enabled ? control.strokeColor : "#e3e7ec"
        border.width: 1
    }

    contentItem: Text {
        text: control.text
        color: control.enabled ? control.labelColor : (control.primary ? "#eef3ff" : "#9aa3af")
        font.pixelSize: 15
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    HoverHandler {
        id: hoverHandler
        enabled: control.enabled
    }

    TapHandler {
        id: tapHandler
        enabled: control.enabled
        onTapped: control.clicked()
    }
}
