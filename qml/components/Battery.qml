// Battery.qml
// Responsive Battery component (keeps original structure and ids, but sizes adapt to battery.width/height)

import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: battery
    property real level: 0.75    // 0..1
    property bool charging: false
    property int warningThreshold: 20
    property int criticalThreshold: 10

    // default size (can be overridden by parent)
    width: 200
    height: 80

    // computed margins / sizes to keep everything proportional
    property real hMargin: Math.max(6, battery.width * 0.04)
    property real vMargin: Math.max(4, battery.height * 0.06)
    property real tipWidth: Math.max(8, battery.width * 0.07)
    property real tipGap: Math.max(2, battery.width * 0.01)

    Rectangle {
        id: shell
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        x: hMargin
        y: vMargin
        width: battery.width - hMargin - tipWidth - tipGap - hMargin
        height: battery.height - vMargin * 2
        radius: Math.max(4, shell.height * 0.12)
        color: "#1e1e1e"
        border.color: "#3a3a3a"
        border.width: Math.max(1, shell.height * 0.06)
        z: 0

        // inner padding
        Rectangle {
            id: inner
            anchors.fill: parent
            anchors.margins: Math.max(4, shell.height * 0.08)
            radius: Math.max(3, inner.height * 0.08)
            color: "transparent"
            clip: true

            // background behind fill
            Rectangle {
                anchors.fill: parent
                color: "#121212"
            }

            // Fill that represents the battery level
            Rectangle {
                id: fill
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                y: 0
                width: Math.max(0, parent.width * Math.min(1, battery.level))
                height: parent.height
                radius: Math.max(2, fill.height * 0.06)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: fillGradientStart }
                    GradientStop { position: 1.0; color: fillGradientEnd }
                }
            }
        }
    }

    // Battery tip (electrode)
    Rectangle {
        id: tip
        anchors.verticalCenter: shell.verticalCenter
        anchors.left: shell.right
        anchors.leftMargin: tipGap
        width: tipWidth
        height: shell.height * 0.45
        radius: Math.max(2, height * 0.03)
        color: "#2a2a2a"
        border.color: "#3a3a3a"
        border.width: Math.max(1, shell.height * 0.04)
    }

    // Percentage text (responsive)
    Text {
        id: percent
        anchors.centerIn: shell
        text: Math.round(battery.level * 100) + "%"
        font.pixelSize: Math.max(10, shell.height * 0.45)
        font.bold: true
        color: "white"
        z: 10
    }

    // Charging bolt (appears when charging), scaled with shell
    Item {
        id: chargingIcon
        anchors.verticalCenter: shell.verticalCenter
        anchors.right: shell.right
        anchors.rightMargin: Math.max(6, shell.width * 0.04)
        width: Math.max(18, shell.height * 0.9)
        height: Math.max(18, shell.height * 0.9)
        visible: battery.charging
        opacity: battery.charging ? 1 : 0

        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0,0,width,height);
                ctx.beginPath();
                ctx.moveTo(width*0.55, height*0.12);
                ctx.lineTo(width*0.4, height*0.55);
                ctx.lineTo(width*0.6, height*0.55);
                ctx.lineTo(width*0.45, height*0.88);
                ctx.lineTo(width*0.68, height*0.45);
                ctx.lineTo(width*0.48, height*0.45);
                ctx.closePath();
                ctx.fillStyle = "#ffd24d";
                ctx.fill();
            }
        }

        SequentialAnimation on scale {
            loops: Animation.Infinite
            NumberAnimation { to: 1.08; duration: 600; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
        }
    }

    // color logic for fill gradient
    property color fillGradientStart: (battery.level * 100 <= battery.criticalThreshold) ? Qt.rgba(0.8,0.12,0.12,1)
                                        : (battery.level * 100 <= battery.warningThreshold) ? Qt.rgba(0.92,0.57,0.12,1)
                                        : Qt.rgba(0.18,0.9,0.36,1)

    property color fillGradientEnd: (battery.level * 100 <= battery.criticalThreshold) ? Qt.rgba(0.6,0.08,0.08,1)
                                      : (battery.level * 100 <= battery.warningThreshold) ? Qt.rgba(0.8,0.4,0.08,1)
                                      : Qt.rgba(0.06,0.6,0.24,1)

    // Smooth animate fill width when level changes
    Behavior on level {
        NumberAnimation { duration: 350; easing.type: Easing.OutQuad }
    }
}
