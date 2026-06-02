import QtQuick

Rectangle {
    id: view

    property var rawPoints: []
    property var calibratedPoints: []
    property real yaw: -36
    property real pitch: 24
    property real roll: 60.76
    property real zoom: 1.0

    implicitWidth: 640
    implicitHeight: 320
    color: "#fbfcfd"
    border.color: "#d8dde4"
    border.width: 1
    radius: 4
    clip: true

    function resetView() {
        yaw = -36
        pitch = 24
        roll = 60.76
        zoom = 1.0
        canvas.requestPaint()
    }

    onRawPointsChanged: canvas.requestPaint()
    onCalibratedPointsChanged: canvas.requestPaint()
    onYawChanged: canvas.requestPaint()
    onPitchChanged: canvas.requestPaint()
    onRollChanged: canvas.requestPaint()
    onZoomChanged: canvas.requestPaint()

    Canvas {
        id: canvas

        anchors.fill: parent
        anchors.margins: 1

        function rotatePoint(point) {
            var yawRadians = view.yaw * Math.PI / 180
            var pitchRadians = view.pitch * Math.PI / 180
            var rollRadians = view.roll * Math.PI / 180
            var yawCosine = Math.cos(yawRadians)
            var yawSine = Math.sin(yawRadians)
            var pitchCosine = Math.cos(pitchRadians)
            var pitchSine = Math.sin(pitchRadians)
            var rollCosine = Math.cos(rollRadians)
            var rollSine = Math.sin(rollRadians)
            var yawX = yawCosine * point.x + yawSine * point.z
            var yawZ = -yawSine * point.x + yawCosine * point.z
            var pitchY = pitchCosine * point.y - pitchSine * yawZ
            return {
                "x": rollCosine * yawX - rollSine * pitchY,
                "y": rollSine * yawX + rollCosine * pitchY,
                "z": pitchSine * point.y + pitchCosine * yawZ
            }
        }

        function coordinate(point, name, index) {
            if (point[name] !== undefined) {
                return Number(point[name])
            }
            return Number(point[index])
        }

        function calculateSeriesExtent(points) {
            var minimum = { "x": Infinity, "y": Infinity, "z": Infinity }
            var maximum = { "x": -Infinity, "y": -Infinity, "z": -Infinity }

            for (var index = 0; index < points.length; ++index) {
                var point = points[index]
                var x = coordinate(point, "x", 0)
                var y = coordinate(point, "y", 1)
                var z = coordinate(point, "z", 2)
                if (!isFinite(x) || !isFinite(y) || !isFinite(z)) {
                    continue
                }
                minimum.x = Math.min(minimum.x, x)
                minimum.y = Math.min(minimum.y, y)
                minimum.z = Math.min(minimum.z, z)
                maximum.x = Math.max(maximum.x, x)
                maximum.y = Math.max(maximum.y, y)
                maximum.z = Math.max(maximum.z, z)
            }

            return {
                "valid": isFinite(minimum.x),
                "extent": Math.max(maximum.x - minimum.x,
                                   maximum.y - minimum.y,
                                   maximum.z - minimum.z,
                                   0.001)
            }
        }

        function calculateBounds() {
            var minimum = { "x": 0, "y": 0, "z": 0 }
            var maximum = { "x": 0, "y": 0, "z": 0 }
            var rawStats = calculateSeriesExtent(view.rawPoints)
            var calibratedStats = calculateSeriesExtent(view.calibratedPoints)
            var calibratedScale = rawStats.valid && calibratedStats.valid
                ? rawStats.extent / calibratedStats.extent
                : 1.0

            function include(points, displayScale) {
                for (var index = 0; index < points.length; ++index) {
                    var point = points[index]
                    var x = coordinate(point, "x", 0) * displayScale
                    var y = coordinate(point, "y", 1) * displayScale
                    var z = coordinate(point, "z", 2) * displayScale
                    if (!isFinite(x) || !isFinite(y) || !isFinite(z)) {
                        continue
                    }
                    minimum.x = Math.min(minimum.x, x)
                    minimum.y = Math.min(minimum.y, y)
                    minimum.z = Math.min(minimum.z, z)
                    maximum.x = Math.max(maximum.x, x)
                    maximum.y = Math.max(maximum.y, y)
                    maximum.z = Math.max(maximum.z, z)
                }
            }

            include(view.rawPoints, 1.0)
            include(view.calibratedPoints, calibratedScale)

            var center = {
                "x": 0.5 * (minimum.x + maximum.x),
                "y": 0.5 * (minimum.y + maximum.y),
                "z": 0.5 * (minimum.z + maximum.z)
            }
            var extent = Math.max(maximum.x - minimum.x,
                                  maximum.y - minimum.y,
                                  maximum.z - minimum.z,
                                  0.001)
            return {
                "center": center,
                "extent": extent,
                "axisLength": 0.64 * extent,
                "calibratedScale": calibratedScale
            }
        }

        function projectPoint(point, bounds) {
            var rotated = rotatePoint({
                "x": point.x - bounds.center.x,
                "y": point.y - bounds.center.y,
                "z": point.z - bounds.center.z
            })
            var scale = 0.78 * Math.min(width, height) * view.zoom / bounds.extent
            return {
                "x": 0.5 * width + rotated.x * scale,
                "y": 0.5 * height - rotated.y * scale,
                "z": rotated.z
            }
        }

        function drawAxis(context, bounds, axis, color, label) {
            var start = { "x": 0, "y": 0, "z": 0 }
            var end = { "x": 0, "y": 0, "z": 0 }
            start[axis] = -bounds.axisLength
            end[axis] = bounds.axisLength
            var projectedStart = projectPoint(start, bounds)
            var projectedEnd = projectPoint(end, bounds)
            context.beginPath()
            context.moveTo(projectedStart.x, projectedStart.y)
            context.lineTo(projectedEnd.x, projectedEnd.y)
            context.strokeStyle = color
            context.lineWidth = 1
            context.stroke()
            context.fillStyle = color
            context.font = "11px sans-serif"
            context.fillText(label, projectedEnd.x + 5, projectedEnd.y - 4)
        }

        function appendRenderablePoints(renderPoints, points, color, radius, displayScale) {
            for (var index = 0; index < points.length; ++index) {
                var point = points[index]
                var x = coordinate(point, "x", 0) * displayScale
                var y = coordinate(point, "y", 1) * displayScale
                var z = coordinate(point, "z", 2) * displayScale
                if (!isFinite(x) || !isFinite(y) || !isFinite(z)) {
                    continue
                }
                renderPoints.push({
                    "x": x,
                    "y": y,
                    "z": z,
                    "color": color,
                    "radius": radius
                })
            }
        }

        onPaint: {
            var context = getContext("2d")
            context.clearRect(0, 0, width, height)
            context.fillStyle = view.color
            context.fillRect(0, 0, width, height)

            var bounds = calculateBounds()
            drawAxis(context, bounds, "x", "#d45757", "X")
            drawAxis(context, bounds, "y", "#47a36b", "Y")
            drawAxis(context, bounds, "z", "#4b70d9", "Z")

            var renderPoints = []
            appendRenderablePoints(renderPoints, view.rawPoints, "#e09a28", 2.2, 1.0)
            appendRenderablePoints(renderPoints, view.calibratedPoints, "#2458f2", 2.0,
                                   bounds.calibratedScale)
            renderPoints.sort(function(left, right) {
                var leftDepth = rotatePoint(left).z
                var rightDepth = rotatePoint(right).z
                return leftDepth - rightDepth
            })

            for (var index = 0; index < renderPoints.length; ++index) {
                var point = renderPoints[index]
                var projected = projectPoint(point, bounds)
                context.beginPath()
                context.arc(projected.x, projected.y, point.radius, 0, 2 * Math.PI)
                context.fillStyle = point.color
                context.globalAlpha = 0.72
                context.fill()
            }
            context.globalAlpha = 1.0

            if (view.rawPoints.length === 0) {
                context.fillStyle = "#7b8491"
                context.font = "12px sans-serif"
                context.textAlign = "center"
                context.fillText("Collect magnetometer samples to populate the 3D view",
                                 0.5 * width,
                                 0.5 * height)
                context.textAlign = "left"
            }
        }
    }

    MouseArea {
        id: mouseArea

        property real previousX: 0
        property real previousY: 0

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

        onPressed: function(mouse) {
            previousX = mouse.x
            previousY = mouse.y
        }

        onPositionChanged: function(mouse) {
            if (!pressed) {
                return
            }
            view.yaw += mouse.x - previousX
            view.pitch = Math.max(-89, Math.min(89, view.pitch + mouse.y - previousY))
            previousX = mouse.x
            previousY = mouse.y
        }

        onWheel: function(wheel) {
            var factor = wheel.angleDelta.y > 0 ? 1 / 0.98 : 0.98
            view.zoom = Math.max(0.35, Math.min(3.5, view.zoom * factor))
            wheel.accepted = true
        }
    }
}
