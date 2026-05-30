import QtQuick

Item {
    id: root

    property real roll: 0
    property real pitch: 0
    property real heading: 0
    property bool showYaw: false

    implicitWidth: 360
    implicitHeight: 360

    function clamp(value, minimum, maximum) {
        return Math.max(minimum, Math.min(maximum, value))
    }

    function normalizedHeading() {
        var value = Number(root.heading) % 360
        return value < 0 ? value + 360 : value
    }

    function headingText() {
        var value = Math.round(normalizedHeading()).toString()
        while (value.length < 3) {
            value = "0" + value
        }
        return value + "\u00b0"
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
            var outerRadius = size / 2 - 2
            var bezelRadius = outerRadius - 8
            var faceRadius = outerRadius - 18
            var scale = size / 360
            var pitchScale = faceRadius / 48
            var pitchOffset = root.clamp(Number(root.pitch), -90, 90) * pitchScale
            var extent = faceRadius * 3.4
            var rollRadians = -Number(root.roll) * Math.PI / 180

            ctx.clearRect(0, 0, width, height)

            ctx.fillStyle = "#15191d"
            ctx.beginPath()
            ctx.arc(cx, cy, outerRadius, 0, Math.PI * 2)
            ctx.fill()

            ctx.fillStyle = "#3e4549"
            ctx.beginPath()
            ctx.arc(cx, cy, bezelRadius, 0, Math.PI * 2)
            ctx.fill()

            ctx.fillStyle = "#101315"
            ctx.beginPath()
            ctx.arc(cx, cy, faceRadius + 4, 0, Math.PI * 2)
            ctx.fill()

            ctx.save()
            ctx.beginPath()
            ctx.arc(cx, cy, faceRadius, 0, Math.PI * 2)
            ctx.clip()

            ctx.translate(cx, cy)
            ctx.rotate(rollRadians)

            var skyGradient = ctx.createLinearGradient(0, -extent, 0, pitchOffset)
            skyGradient.addColorStop(0, "#3068ab")
            skyGradient.addColorStop(1, "#5e9ee1")
            ctx.fillStyle = skyGradient
            ctx.fillRect(-extent, -extent + pitchOffset, extent * 2, extent)

            var groundGradient = ctx.createLinearGradient(0, pitchOffset, 0, extent)
            groundGradient.addColorStop(0, "#86a83e")
            groundGradient.addColorStop(1, "#527323")
            ctx.fillStyle = groundGradient
            ctx.fillRect(-extent, pitchOffset, extent * 2, extent)

            ctx.strokeStyle = "#d8deaf"
            ctx.lineWidth = Math.max(2, 2 * scale)
            root.drawLine(ctx, -extent, pitchOffset, extent, pitchOffset)

            ctx.font = "bold " + Math.max(10, Math.round(12 * scale)) + "px sans-serif"
            ctx.textAlign = "center"
            ctx.textBaseline = "middle"

            for (var angle = -80; angle <= 80; angle += 5) {
                if (angle === 0) {
                    continue
                }

                var y = pitchOffset - angle * pitchScale
                var isMajor = angle % 10 === 0
                var halfWidth = (isMajor ? 29 : 16) * scale
                ctx.strokeStyle = "rgba(255, 255, 255, 0.92)"
                ctx.lineWidth = Math.max(1, (isMajor ? 2 : 1) * scale)
                root.drawLine(ctx, -halfWidth, y, halfWidth, y)

                if (isMajor) {
                    ctx.fillStyle = "rgba(255, 255, 255, 0.94)"
                    ctx.fillText(Math.abs(angle).toString(), -halfWidth - 18 * scale, y)
                    ctx.fillText(Math.abs(angle).toString(), halfWidth + 18 * scale, y)
                }
            }

            ctx.restore()

            ctx.strokeStyle = "#aeb8b9"
            ctx.lineWidth = Math.max(2, 2 * scale)
            ctx.beginPath()
            ctx.arc(cx, cy, faceRadius, 0, Math.PI * 2)
            ctx.stroke()

            var tickAngles = [-60, -45, -30, -20, -10, 0, 10, 20, 30, 45, 60]
            for (var i = 0; i < tickAngles.length; ++i) {
                var tickAngle = tickAngles[i]
                var radians = tickAngle * Math.PI / 180
                var tickOuter = faceRadius - 5 * scale
                var tickLength = tickAngle === 0 || Math.abs(tickAngle) === 30 || Math.abs(tickAngle) === 60
                                 ? 14 * scale : 9 * scale
                var tickInner = tickOuter - tickLength

                ctx.strokeStyle = "rgba(255, 255, 255, 0.88)"
                ctx.lineWidth = Math.max(1, 2 * scale)
                root.drawLine(ctx,
                              cx + Math.sin(radians) * tickInner,
                              cy - Math.cos(radians) * tickInner,
                              cx + Math.sin(radians) * tickOuter,
                              cy - Math.cos(radians) * tickOuter)
            }

            ctx.fillStyle = "#f05a3d"
            ctx.beginPath()
            ctx.moveTo(cx, cy - faceRadius + 7 * scale)
            ctx.lineTo(cx - 7 * scale, cy - faceRadius + 22 * scale)
            ctx.lineTo(cx + 7 * scale, cy - faceRadius + 22 * scale)
            ctx.closePath()
            ctx.fill()

            ctx.strokeStyle = "#353a35"
            ctx.lineWidth = Math.max(5, 6 * scale)
            ctx.lineCap = "round"
            root.drawLine(ctx, cx - 65 * scale, cy, cx - 18 * scale, cy)
            root.drawLine(ctx, cx + 18 * scale, cy, cx + 65 * scale, cy)
            root.drawLine(ctx, cx - 18 * scale, cy, cx, cy + 10 * scale)
            root.drawLine(ctx, cx, cy + 10 * scale, cx + 18 * scale, cy)

            ctx.strokeStyle = "#f5f1df"
            ctx.lineWidth = Math.max(2, 3 * scale)
            root.drawLine(ctx, cx - 65 * scale, cy, cx - 18 * scale, cy)
            root.drawLine(ctx, cx + 18 * scale, cy, cx + 65 * scale, cy)
            root.drawLine(ctx, cx - 18 * scale, cy, cx, cy + 10 * scale)
            root.drawLine(ctx, cx, cy + 10 * scale, cx + 18 * scale, cy)

            ctx.fillStyle = "#f05a3d"
            ctx.beginPath()
            ctx.arc(cx, cy, Math.max(4, 5 * scale), 0, Math.PI * 2)
            ctx.fill()

            if (root.showYaw) {
                var boxWidth = 56 * scale
                var boxHeight = 25 * scale
                var boxX = cx - boxWidth / 2
                var boxY = cy + faceRadius - boxHeight - 13 * scale

                ctx.fillStyle = "rgba(15, 20, 22, 0.74)"
                ctx.fillRect(boxX, boxY, boxWidth, boxHeight)
                ctx.strokeStyle = "rgba(255, 255, 255, 0.55)"
                ctx.lineWidth = Math.max(1, scale)
                ctx.strokeRect(boxX, boxY, boxWidth, boxHeight)

                ctx.fillStyle = "#ffffff"
                ctx.font = "bold " + Math.max(10, Math.round(12 * scale)) + "px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText(root.headingText(), cx, boxY + boxHeight / 2)
            }
        }

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
    }

    onRollChanged: gauge.requestPaint()
    onPitchChanged: gauge.requestPaint()
    onHeadingChanged: gauge.requestPaint()
    onShowYawChanged: gauge.requestPaint()
}
