import QtQuick

Item {
    id: control

    property string styleName: "cancel-button"
    property string text: ""
    property int fontPixelSize: 11
    property bool interactive: true
    property bool progressEnabled: false
    property real progressFrom: 0
    property real progressTo: 100
    property real progressValue: 0
    property color progressColor: primary ? "#4c75f2" : "#d7e2ff"
    property color progressTrackColor: primary ? "#9fb5f8" : "#f3f6ff"
    signal clicked()

    implicitWidth: 80
    implicitHeight: 30
    activeFocusOnTab: enabled
    opacity: enabled ? 1.0 : 0.55

    readonly property bool primary: styleName === "primary-button"
    readonly property real progressRatio: {
        var range = progressTo - progressFrom
        if (range <= 0) {
            return 0
        }
        return Math.max(0, Math.min(1, (progressValue - progressFrom) / range))
    }
    readonly property color fillColor: primary
                                      ? (mouseArea.pressed ? "#3f67dc" : "#4c75f2")
                                      : (mouseArea.pressed ? "#f4f6f8" : "#ffffff")
    readonly property color strokeColor: primary ? "#4c75f2"
                                                  : (mouseArea.containsMouse ? "#bfc7d1" : "#cfd5dd")
    readonly property color labelColor: primary ? "#ffffff" : "#2d333b"

    Accessible.role: Accessible.Button
    Accessible.name: text

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 4
        color: control.enabled
               ? (control.progressEnabled ? control.progressTrackColor : control.fillColor)
               : (control.primary ? "#9fb5f8" : "#fbfcfd")
        border.color: control.enabled ? control.strokeColor : "#e3e7ec"
        border.width: 1
        clip: true

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: Math.round(parent.width * control.progressRatio)
            color: control.progressColor
            visible: control.enabled && control.progressEnabled
        }
    }

    Text {
        anchors.fill: parent
        text: control.text
        color: control.enabled ? control.labelColor : (control.primary ? "#eef3ff" : "#9aa3af")
        font.pixelSize: control.fontPixelSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: control.enabled && control.interactive
        hoverEnabled: true
        cursorShape: control.enabled && control.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: control.clicked()
    }

    Keys.onReturnPressed: if (enabled && interactive) clicked()
    Keys.onSpacePressed: if (enabled && interactive) clicked()
}
