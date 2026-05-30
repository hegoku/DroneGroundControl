import QtQuick

Item {
    id: root

    property real heading: 0
    property bool showHeading: true

    implicitWidth: 220
    implicitHeight: 220

    function normalizedHeading() {
        var value = Number(root.heading) % 360
        return value < 0 ? value + 360 : value
    }

    function headingText() {
        var value = Math.round(normalizedHeading()).toString()
        while (value.length < 3) {
            value = "0" + value
        }
        return value
    }

    function drawLine(ctx, x1, y1, x2, y2) {
        ctx.beginPath()
        ctx.moveTo(x1, y1)
        ctx.lineTo(x2, y2)
        ctx.stroke()
    }

    Canvas {
        id: gauge
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d")
            var size = Math.min(width, height)
            var cx = width / 2
            var cy = height / 2
            var scale = size / 220
            var outerRadius = size / 2 - 2
            var bezelRadius = outerRadius - 7 * scale
            var faceRadius = outerRadius - 14 * scale

            ctx.clearRect(0, 0, width, height)

            ctx.fillStyle = "#171a1d"
            ctx.beginPath()
            ctx.arc(cx, cy, outerRadius, 0, Math.PI * 2)
            ctx.fill()

            ctx.strokeStyle = "#596166"
            ctx.lineWidth = Math.max(2, 3 * scale)
            ctx.beginPath()
            ctx.arc(cx, cy, bezelRadius, 0, Math.PI * 2)
            ctx.stroke()

            ctx.fillStyle = "#111315"
            ctx.beginPath()
            ctx.arc(cx, cy, faceRadius, 0, Math.PI * 2)
            ctx.fill()

            for (var angle = 0; angle < 360; angle += 5) {
                var radians = angle * Math.PI / 180
                var isCardinal = angle % 90 === 0
                var isMajor = angle % 30 === 0
                var tickOuter = faceRadius - 3 * scale
                var tickLength = (isCardinal ? 15 : isMajor ? 11 : 7) * scale
                var tickInner = tickOuter - tickLength

                ctx.strokeStyle = isMajor ? "#f1f2ef" : "#b9bec1"
                ctx.lineWidth = Math.max(1, (isMajor ? 2 : 1) * scale)
                root.drawLine(ctx,
                              cx + Math.sin(radians) * tickInner,
                              cy - Math.cos(radians) * tickInner,
                              cx + Math.sin(radians) * tickOuter,
                              cy - Math.cos(radians) * tickOuter)
            }

            ctx.fillStyle = "#dce1e2"
            ctx.font = "bold " + Math.max(11, Math.round(16 * scale)) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"

            var labelRadius = faceRadius - 27 * scale
            ctx.fillText("N", cx, cy - labelRadius)
            ctx.fillText("E", cx + labelRadius, cy)
            ctx.fillText("S", cx, cy + labelRadius)
            ctx.fillText("W", cx - labelRadius, cy)

            ctx.save()
            ctx.translate(cx, cy)
            ctx.rotate(root.normalizedHeading() * Math.PI / 180)

            ctx.fillStyle = "#ce3f32"
            ctx.beginPath()
            ctx.moveTo(0, -faceRadius * 0.60)
            ctx.lineTo(-11 * scale, 14 * scale)
            ctx.lineTo(0, 7 * scale)
            ctx.lineTo(11 * scale, 14 * scale)
            ctx.closePath()
            ctx.fill()

            ctx.fillStyle = "#8a2a24"
            ctx.beginPath()
            ctx.moveTo(0, faceRadius * 0.56)
            ctx.lineTo(-8 * scale, 4 * scale)
            ctx.lineTo(8 * scale, 4 * scale)
            ctx.closePath()
            ctx.fill()
            ctx.restore()

            ctx.fillStyle = "#d8dcda"
            ctx.beginPath()
            ctx.arc(cx, cy, Math.max(3, 4 * scale), 0, Math.PI * 2)
            ctx.fill()

            if (root.showHeading) {
                var boxWidth = 62 * scale
                var boxHeight = 31 * scale
                var boxX = cx - boxWidth / 2
                var boxY = cy - boxHeight / 2

                ctx.fillStyle = "rgba(37, 42, 45, 0.94)"
                ctx.fillRect(boxX, boxY, boxWidth, boxHeight)
                ctx.strokeStyle = "#c0c6c7"
                ctx.lineWidth = Math.max(1, 2 * scale)
                ctx.strokeRect(boxX, boxY, boxWidth, boxHeight)

                ctx.fillStyle = "#e3e6e4"
                ctx.font = "bold " + Math.max(12, Math.round(17 * scale)) + "px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(root.headingText(), cx, cy)
            }

            ctx.strokeStyle = "#adb5b8"
            ctx.lineWidth = Math.max(1, 2 * scale)
            ctx.beginPath()
            ctx.arc(cx, cy, faceRadius, 0, Math.PI * 2)
            ctx.stroke()
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    onHeadingChanged: gauge.requestPaint()
    onShowHeadingChanged: gauge.requestPaint()
}
