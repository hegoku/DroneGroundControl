pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: control

    property int minimumValue: 1000
    property int maximumValue: 2000
    property int xValue: 1500
    property int yValue: 1500
    property int defaultXValue: 1500
    property int defaultYValue: 1500
    property bool returnXToDefaultOnRelease: false
    property bool returnYToDefaultOnRelease: false
    property string xLabel: "Roll"
    property string yLabel: "Pitch"
    property bool interactive: true
    signal valuesEdited(int xValue, int yValue)

    implicitWidth: 420
    implicitHeight: 360
    opacity: enabled ? 1.0 : 0.55

    readonly property int range: Math.max(1, maximumValue - minimumValue)

    function clampValue(value) {
        return Math.max(minimumValue, Math.min(maximumValue, Math.round(Number(value))))
    }

    function valueToRatio(value) {
        return (clampValue(value) - minimumValue) / range
    }

    function updateFromPoint(pointX, pointY) {
        var localX = Math.max(0, Math.min(padCanvas.width, pointX))
        var localY = Math.max(0, Math.min(padCanvas.height, pointY))
        var nextX = clampValue(minimumValue + (localX / padCanvas.width) * range)
        var nextY = clampValue(maximumValue - (localY / padCanvas.height) * range)
        valuesEdited(nextX, nextY)
    }

    function releaseToDefaults() {
        var nextX = returnXToDefaultOnRelease ? clampValue(defaultXValue) : xValue
        var nextY = returnYToDefaultOnRelease ? clampValue(defaultYValue) : yValue
        if (nextX !== xValue || nextY !== yValue) {
            valuesEdited(nextX, nextY)
        }
    }

    Rectangle {
        id: padFrame
        width: Math.min(control.width, control.height)
        height: width
        radius: 10
        color: "#ffffff"
        border.color: "#e1e5eb"
        border.width: 1
        anchors.centerIn: parent
        clip: true

        Text {
            id: yTitle
            text: control.yLabel
            color: "#2f343b"
            font.pixelSize: 16
            anchors.horizontalCenter: padCanvas.horizontalCenter
            anchors.bottom: yValueText.top
            anchors.bottomMargin: 2
        }

        Text {
            id: yValueText
            text: control.yValue
            color: "#18325f"
            font.pixelSize: 18
            anchors.horizontalCenter: yTitle.horizontalCenter
            anchors.bottom: topMaxLabel.top
            anchors.bottomMargin: 2
        }

        Text {
            id: xTitle
            text: control.xLabel
            color: "#2f343b"
            font.pixelSize: 16
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.bottom: xValueText.top
            anchors.bottomMargin: 2
        }

        Text {
            id: xValueText
            text: control.xValue
            color: "#18325f"
            font.pixelSize: 18
            anchors.left: xTitle.left
            anchors.verticalCenter: padCanvas.verticalCenter
            anchors.verticalCenterOffset: 20
        }

        Canvas {
            id: padCanvas
            anchors.fill: parent
            anchors.leftMargin: 104
            anchors.rightMargin: 60
            anchors.topMargin: 68
            anchors.bottomMargin: 50

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                var cx = width / 2
                var cy = height / 2
                var radius = Math.min(width, height) / 2

                ctx.strokeStyle = "#e7ebef"
                ctx.lineWidth = 1
                ctx.setLineDash([])
                ctx.beginPath()
                ctx.arc(cx, cy, radius, 0, Math.PI * 2)
                ctx.stroke()

                ctx.strokeStyle = "#d4dbe2"
                ctx.setLineDash([2, 3])
                ctx.beginPath()
                ctx.arc(cx, cy, radius * 0.72, 0, Math.PI * 2)
                ctx.stroke()

                ctx.strokeStyle = "#e1e6eb"
                ctx.beginPath()
                ctx.arc(cx, cy, radius * 0.48, 0, Math.PI * 2)
                ctx.stroke()

                ctx.setLineDash([])
                ctx.strokeStyle = "#b6bdc6"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(cx, 0)
                ctx.lineTo(cx, height)
                ctx.moveTo(0, cy)
                ctx.lineTo(width, cy)
                ctx.stroke()

                ctx.strokeStyle = "#aeb6bf"
                ctx.beginPath()
                ctx.moveTo(cx - 5, 0)
                ctx.lineTo(cx + 5, 0)
                ctx.moveTo(cx - 5, height)
                ctx.lineTo(cx + 5, height)
                ctx.moveTo(0, cy - 5)
                ctx.lineTo(0, cy + 5)
                ctx.moveTo(width, cy - 5)
                ctx.lineTo(width, cy + 5)
                ctx.stroke()
            }
        }

        Text {
            id: topMaxLabel
            text: control.maximumValue
            color: "#6b7280"
            font.pixelSize: 14
            anchors.horizontalCenter: padCanvas.horizontalCenter
            anchors.bottom: padCanvas.top
            anchors.bottomMargin: 8
        }

        Text {
            text: control.minimumValue
            color: "#6b7280"
            font.pixelSize: 14
            anchors.horizontalCenter: padCanvas.horizontalCenter
            anchors.top: padCanvas.bottom
            anchors.topMargin: 8
        }

        Text {
            id: leftMinLabel
            text: control.minimumValue
            color: "#6b7280"
            font.pixelSize: 14
            anchors.right: padCanvas.left
            anchors.rightMargin: 8
            anchors.verticalCenter: padCanvas.verticalCenter
        }

        Text {
            text: control.maximumValue
            color: "#6b7280"
            font.pixelSize: 14
            anchors.left: padCanvas.right
            anchors.leftMargin: 8
            anchors.verticalCenter: padCanvas.verticalCenter
        }

        Rectangle {
            id: thumbShadow
            width: 38
            height: 38
            radius: 19
            x: padCanvas.x + control.valueToRatio(control.xValue) * padCanvas.width - width / 2 + 3
            y: padCanvas.y + (1 - control.valueToRatio(control.yValue)) * padCanvas.height - height / 2 + 4
            color: "#66000000"
            opacity: 0.16
        }

        Rectangle {
            width: 38
            height: 38
            radius: 19
            x: padCanvas.x + control.valueToRatio(control.xValue) * padCanvas.width - width / 2
            y: padCanvas.y + (1 - control.valueToRatio(control.yValue)) * padCanvas.height - height / 2
            color: "#ffffff"
            border.color: "#d7dce4"
            border.width: 1

            Rectangle {
                width: 26
                height: 26
                radius: 13
                color: "#3f6fec"
                anchors.centerIn: parent
            }
        }

        MouseArea {
            anchors.fill: padCanvas
            enabled: control.enabled && control.interactive
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onPressed: function(mouse) {
                control.updateFromPoint(mouse.x, mouse.y)
            }
            onPositionChanged: function(mouse) {
                if (pressed) {
                    control.updateFromPoint(mouse.x, mouse.y)
                }
            }
            onReleased: control.releaseToDefaults()
            onCanceled: control.releaseToDefaults()
        }
    }
}
