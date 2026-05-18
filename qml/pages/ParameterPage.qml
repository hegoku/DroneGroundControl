import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f4f6f8"

    property var store: droneSession.parameterStore
    property bool reading: false
    property int targetCount: 0
    property int completedCount: 0
    property int currentParameterId: -1
    property int failedCount: 0
    property string statusMessage: "Ready"
    property var columnWidths: [80, 210, 150, 110, 110, 320]
    property var minimumColumnWidths: [60, 120, 100, 80, 90, 140]
    property real lastTableWidth: 0

    function totalColumnWidth(widths) {
        var total = 0
        for (var i = 0; i < widths.length; ++i) {
            total += widths[i]
        }
        return total
    }

    function columnWidth(column) {
        return columnWidths[column]
    }

    function setDefaultColumnWidths(width) {
        var proportions = [0.08, 0.16, 0.24, 0.12, 0.12, 0.28]
        var widths = []
        var used = 0
        for (var i = 0; i < proportions.length - 1; ++i) {
            widths[i] = Math.max(minimumColumnWidths[i], Math.round(width * proportions[i]))
            used += widths[i]
        }
        widths[5] = Math.max(minimumColumnWidths[5], width - used)
        columnWidths = widths
        lastTableWidth = width
    }

    function fitColumnsToWidth(width) {
        if (width <= 0) {
            return
        }

        if (lastTableWidth <= 0) {
            setDefaultColumnWidths(width)
            return
        }

        var widths = columnWidths.slice()
        var delta = width - totalColumnWidth(widths)
        if (delta >= 0) {
            widths[5] += delta
        } else {
            for (var i = widths.length - 1; i >= 0 && delta < 0; --i) {
                var shrink = Math.min(widths[i] - minimumColumnWidths[i], -delta)
                widths[i] -= shrink
                delta += shrink
            }
        }
        columnWidths = widths
        lastTableWidth = width
    }

    function resizeColumn(column, delta) {
        if (column < 0 || column >= columnWidths.length - 1) {
            return
        }

        var widths = columnWidths.slice()
        var left = widths[column]
        var right = widths[column + 1]
        var clampedDelta = Math.max(minimumColumnWidths[column] - left,
                                    Math.min(delta, right - minimumColumnWidths[column + 1]))

        if (clampedDelta === 0) {
            return
        }

        widths[column] = left + clampedDelta
        widths[column + 1] = right - clampedDelta
        columnWidths = widths
    }

    function valueText(value, valueHex, hasValue) {
        if (!hasValue) {
            return ""
        }
        if (value === undefined || value === null || String(value).length === 0) {
            return valueHex
        }
        return String(value)
    }

    function failReadAll(message) {
        reading = false
        currentParameterId = -1
        statusMessage = message
    }

    function finishReadAll() {
        reading = false
        currentParameterId = -1
        if (failedCount > 0) {
            statusMessage = "Read " + completedCount + " of " + targetCount + " parameters, " + failedCount + " failed"
        } else {
            statusMessage = "Read " + completedCount + " parameters"
        }
    }

    function markParameterDone(parameterId, failed) {
        currentParameterId = parameterId
        completedCount += 1
        if (failed) {
            failedCount += 1
        }

        if (completedCount >= targetCount) {
            finishReadAll()
        } else {
            statusMessage = "Read " + completedCount + " of " + targetCount + " parameters"
        }
    }

    function readAll() {
        if (reading) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before reading parameters"
            return
        }

        reading = true
        targetCount = 0
        completedCount = 0
        failedCount = 0
        currentParameterId = -1
        statusMessage = "Reading parameter count"

        droneSession.requestParameterCount(function(result) {
            var count = Number(result.frame.value)
            if (!isFinite(count) || count < 0) {
                failReadAll("Parameter count response is invalid")
                return
            }

            targetCount = Math.floor(count)
            if (targetCount === 0) {
                finishReadAll()
                return
            }

            statusMessage = "Queued " + targetCount + " parameter reads"
            for (var i = 0; i < targetCount; ++i) {
                let parameterId = i
                store.refreshParameterInfo(parameterId, function() {
                    store.refreshParameterValue(parameterId, function() {
                        markParameterDone(parameterId, false)
                    }, function(error) {
                        console.warn("Parameter value read failed:", parameterId, error.reason)
                        markParameterDone(parameterId, true)
                    }, "ParameterPage")
                }, function(error) {
                    console.warn("Parameter info read failed:", parameterId, error.reason)
                    markParameterDone(parameterId, true)
                }, "ParameterPage")
            }
        }, function(error) {
            failReadAll("Parameter count failed: " + error.reason)
        })
    }

    Rectangle {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 58
        color: "#ffffff"
        border.color: "#d7dce3"
        border.width: 1

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12

            Button {
                id: readButton
                width: 92
                height: 32
                text: reading ? "Reading" : "Read All"
                enabled: droneSession.isOpen && !reading
                onClicked: readAll()
            }

            ProgressBar {
                width: 180
                height: 12
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: Math.max(targetCount, 1)
                value: completedCount
                visible: reading || completedCount > 0
            }

            Text {
                width: 420
                anchors.verticalCenter: parent.verticalCenter
                text: statusMessage
                color: reading ? "#1f5fbf" : "#4b5563"
                font.pixelSize: 14
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        id: tableFrame
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 18
        color: "#ffffff"
        border.color: "#c7cdd4"
        border.width: 1
        clip: true

        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 36
            color: "#edf0f4"
            border.color: "#d9dee5"
            border.width: 1

            Row {
                anchors.fill: parent

                HeaderCell { width: root.columnWidth(0); text: "ID"; resizeColumn: 0 }
                HeaderCell { width: root.columnWidth(1); text: "Name"; resizeColumn: 1 }
                HeaderCell { width: root.columnWidth(2); text: "Value"; resizeColumn: 2 }
                HeaderCell { width: root.columnWidth(3); text: "Type"; resizeColumn: 3 }
                HeaderCell { width: root.columnWidth(4); text: "Status"; resizeColumn: 4 }
                HeaderCell { width: root.columnWidth(5); text: "Description"; showDivider: false }
            }
        }

        ListView {
            id: tableView
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            model: root.store
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 1200
            reuseItems: true

            onWidthChanged: root.fitColumnsToWidth(width)

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: rowItem
                width: tableView.width
                height: 32
                color: hoverHandler.hovered ? "#eef4ff" : (index % 2 === 0 ? "#ffffff" : "#fbfcfd")

                Row {
                    anchors.fill: parent

                    BodyCell {
                        width: root.columnWidth(0)
                        text: String(model.parameterId)
                        textColor: model.busy ? "#1f5fbf" : "#111827"
                    }
                    BodyCell {
                        width: root.columnWidth(1)
                        text: model.name && model.name.length > 0 ? model.name : "-"
                        textColor: model.hasDefinition ? "#111827" : "#6b7280"
                    }
                    BodyCell {
                        width: root.columnWidth(2)
                        text: root.valueText(model.value, model.valueHex, model.hasValue)
                        textColor: model.hasValue ? "#0f5132" : "#6b7280"
                    }
                    BodyCell {
                        width: root.columnWidth(3)
                        text: model.hasDefinition ? model.typeName : "-"
                    }
                    BodyCell {
                        width: root.columnWidth(4)
                        text: model.busy ? model.busyReason : (model.lastError.length > 0 ? "Error" : "Ready")
                        textColor: model.busy ? "#1f5fbf" : (model.lastError.length > 0 ? "#b42318" : "#4b5563")
                    }
                    BodyCell {
                        width: root.columnWidth(5)
                        text: model.lastError.length > 0 ? model.lastError : model.description
                        textColor: model.lastError.length > 0 ? "#b42318" : "#374151"
                    }
                }

                HoverHandler {
                    id: hoverHandler
                }
            }
        }
    }

    component HeaderCell: Item {
        property alias text: title.text
        property bool showDivider: true
        property int resizeColumn: -1

        height: parent.height

        Text {
            id: title
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: "#262a31"
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }

        Rectangle {
            id: divider
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 5
            anchors.bottomMargin: 5
            width: 1
            color: "#c9ced6"
            visible: showDivider
        }

        MouseArea {
            id: resizeArea
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 10
            enabled: showDivider && resizeColumn >= 0
            hoverEnabled: true
            cursorShape: Qt.SplitHCursor

            property real lastHeaderX: 0

            onPressed: function(mouse) {
                lastHeaderX = mapToItem(header, mouse.x, mouse.y).x
            }

            onPositionChanged: function(mouse) {
                if (!pressed) {
                    return
                }

                var headerX = mapToItem(header, mouse.x, mouse.y).x
                root.resizeColumn(resizeColumn, headerX - lastHeaderX)
                lastHeaderX = headerX
            }

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: parent.height
                color: resizeArea.pressed || resizeArea.containsMouse ? "#4c8bf5" : divider.color
                opacity: resizeArea.pressed || resizeArea.containsMouse ? 1 : 0
            }
        }
    }

    component BodyCell: Item {
        property alias text: value.text
        property color textColor: "#111827"

        height: parent.height

        Text {
            id: value
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: textColor
            font.pixelSize: 15
            elide: Text.ElideRight
        }
    }
}
