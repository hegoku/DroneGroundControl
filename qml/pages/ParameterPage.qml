import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import DroneGroundControl
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#fbfcfd"

    property var store: droneSession.parameterStore
    property bool reading: false
    property bool saving: false
    property int targetCount: 0
    property int completedCount: 0
    property int currentParameterId: -1
    property int failedCount: 0
    property string statusMessage: "Ready"
    property string searchDraftText: ""
    property string searchQuery: ""
    property var columnWidths: [80, 210, 150, 110, 110, 320]
    property var minimumColumnWidths: [60, 120, 100, 80, 90, 140]
    property real lastTableWidth: 0
    readonly property bool flightReady: flight.status === Flight.FLIGHT_STATUS_READY

    FileDialog {
        id: exportDialog
        title: "Export Parameters"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON files (*.json)", "All files (*)"]
        defaultSuffix: "json"
        onAccepted: {
            if (store.exportJson(selectedFile)) {
                statusMessage = "Exported parameters"
            }
        }
    }

    Connections {
        target: root.store

        function onErrorOccurred(message) {
            root.statusMessage = message
        }
    }

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

    function trimFormattedNumber(text) {
        var exponentIndex = Math.max(text.indexOf("e"), text.indexOf("E"))
        if (exponentIndex >= 0) {
            var mantissa = text.slice(0, exponentIndex)
            var exponent = text.slice(exponentIndex + 1)
            mantissa = mantissa.replace(/(\.\d*?)0+$/, "$1").replace(/\.$/, "")
            exponent = exponent.replace(/^\+/, "").replace(/^(-?)0+(\d)/, "$1$2")
            return mantissa + "e" + exponent
        }

        return text.replace(/(\.\d*?)0+$/, "$1").replace(/\.$/, "")
    }

    function numericValueText(value, typeName) {
        if (typeof value !== "number") {
            return String(value)
        }
        if (!isFinite(value)) {
            return String(value)
        }
        if (typeName === "Float") {
            return trimFormattedNumber(value.toPrecision(7))
        }
        if (typeName === "Double") {
            return trimFormattedNumber(value.toPrecision(15))
        }
        return String(value)
    }

    function valueText(value, valueHex, hasValue, typeName) {
        if (!hasValue) {
            return ""
        }
        if (value === undefined || value === null || String(value).length === 0) {
            return valueHex
        }
        return numericValueText(value, typeName)
    }

    function applySearch() {
        searchQuery = searchDraftText.trim()
    }

    function searchTokens() {
        var normalized = searchQuery.trim().toLowerCase()
        if (normalized.length === 0) {
            return []
        }
        return normalized.split(/\s+/)
    }

    function parameterMatchesSearch(name) {
        var tokens = searchTokens()
        if (tokens.length === 0) {
            return true
        }

        var parameterName = String(name || "").toLowerCase()
        for (var i = 0; i < tokens.length; ++i) {
            if (parameterName.indexOf(tokens[i]) < 0) {
                return false
            }
        }
        return true
    }

    function statusColor() {
        if (statusMessage.indexOf("failed") >= 0 || statusMessage.indexOf("failed:") >= 0) {
            return "#ef4444"
        }
        if (reading || saving) {
            return "#4c8bf5"
        }
        return "#61cf6f"
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

    function finishSave() {
        saving = false
        currentParameterId = -1
        if (failedCount > 0) {
            statusMessage = "Saved " + (targetCount - failedCount) + " of " + targetCount + " changed parameters"
        } else {
            statusMessage = "Saved " + targetCount + " changed parameters"
        }
    }

    function markParameterSaved(parameterId, failed) {
        currentParameterId = parameterId
        completedCount += 1
        if (failed) {
            failedCount += 1
        }

        if (completedCount < targetCount) {
            statusMessage = "Saved " + completedCount + " of " + targetCount + " changed parameters"
            return
        }

        if (failedCount > 0) {
            finishSave()
            return
        }

        statusMessage = "Persisting parameter changes"
        droneSession.saveParameters(function() {
            finishSave()
        }, function(error) {
            saving = false
            currentParameterId = -1
            statusMessage = "Parameter persist failed: " + error.reason
        })
    }

    function saveAll() {
        if (saving || reading) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before saving parameters"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before saving parameters"
            return
        }

        var ids = store.dirtyParameterIds()
        if (ids.length === 0) {
            statusMessage = "No parameter changes to save"
            return
        }

        saving = true
        targetCount = ids.length
        completedCount = 0
        failedCount = 0
        currentParameterId = -1
        statusMessage = "Saving " + targetCount + " changed parameters"

        for (var i = 0; i < ids.length; ++i) {
            let parameterId = Number(ids[i])
            store.writeParameter(parameterId, function() {
                markParameterSaved(parameterId, false)
            }, function(error) {
                console.warn("Parameter write failed:", parameterId, error.reason)
                markParameterSaved(parameterId, true)
            }, "ParameterPage")
        }
    }

    function readAll() {
        if (reading || saving) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before reading parameters"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before reading parameters"
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
        height: 48
        color: "transparent"

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            spacing: 16

            StyledButton {
                id: readButton
                width: 96
                height: 32
                styleName: "primary-button"
                text: reading ? "Reading" : "Read All"
                enabled: droneSession.isOpen && root.flightReady && !reading && !saving
                onClicked: readAll()
            }

            StyledButton {
                id: saveButton
                width: 74
                height: 32
                styleName: "primary-button"
                text: saving ? "Saving" : "Save"
                enabled: droneSession.isOpen && root.flightReady && !reading && !saving && store.dirtyCount > 0
                onClicked: saveAll()
            }

            StyledButton {
                id: exportButton
                width: 78
                height: 32
                text: "Export"
                enabled: !reading && !saving
                onClicked: exportDialog.open()
            }

            StyledProgressBar {
                id: operationProgress
                width: 150
                height: 6
                anchors.verticalCenter: parent.verticalCenter
                from: 0
                to: Math.max(targetCount, 1)
                value: completedCount
                progressColor: root.statusColor()
                visible: reading || saving || completedCount > 0
            }

            Row {
                height: 32
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: root.statusColor()
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    width: Math.max(0, toolbar.width - searchControls.width - 525)
                    anchors.verticalCenter: parent.verticalCenter
                    text: statusMessage
                    color: "#2b3036"
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }

        Row {
            id: searchControls
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            height: 32

            Rectangle {
                width: 280
                height: parent.height
                radius: 4
                color: "#ffffff"
                border.color: searchInput.activeFocus ? "#4c8bf5" : "#cfd5dd"
                border.width: 1

                Image {
                    id: searchIcon
                    source: "qrc:/resources/icons/search.svg"
                    width: 15
                    height: 15
                    fillMode: Image.PreserveAspectFit
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                }

                TextInput {
                    id: searchInput
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 34
                    anchors.rightMargin: 10
                    height: parent.height
                    text: root.searchDraftText
                    color: "#111827"
                    selectionColor: "#cfe0ff"
                    selectedTextColor: "#111827"
                    font.pixelSize: 14
                    selectByMouse: true
                    verticalAlignment: TextInput.AlignVCenter

                    onTextChanged: {
                        root.searchDraftText = text
                        root.applySearch()
                    }

                    Keys.onReturnPressed: root.applySearch()
                    Keys.onEnterPressed: root.applySearch()
                }

                Text {
                    anchors.left: searchInput.left
                    anchors.right: searchInput.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Search parameter name"
                    color: "#8a93a0"
                    font.pixelSize: 14
                    elide: Text.ElideRight
                    visible: searchInput.text.length === 0 && !searchInput.activeFocus
                }
            }
        }
    }

    Rectangle {
        id: tableFrame
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 5
        radius: 8
        color: "#ffffff"
        border.color: "#dfe4ea"
        border.width: 1
        clip: true

        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 44
            color: "#f8fafc"
            border.color: "#e2e7ee"
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
                height: matchesSearch ? 32 : 0
                visible: matchesSearch
                color: hoverHandler.hovered ? "#eef4ff" : (index % 2 === 0 ? "#ffffff" : "#fbfcfd")

                readonly property bool matchesSearch: root.parameterMatchesSearch(model.name)

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
                    Item {
                        width: root.columnWidth(2)
                        height: parent.height

                        Rectangle {
                            id: valueEditorFrame
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 5
                            anchors.rightMargin: 5
                            anchors.topMargin: 3
                            anchors.bottomMargin: 3
                            color: valueEditor.enabled ? "#ffffff" : "transparent"
                            border.color: valueEditor.activeFocus ? "#4c8bf5" : (model.dirty ? "#d18a00" : "#d7dce3")
                            border.width: valueEditor.enabled || model.dirty ? 1 : 0
                            radius: 3

                            TextInput {
                                id: valueEditor
                                anchors.fill: parent
                                anchors.margins: 4
                                text: root.valueText(model.value, model.valueHex, model.hasValue, model.typeName)
                                enabled: model.editable && model.hasDefinition && model.hasValue && !reading && !saving
                                selectByMouse: true
                                font.pixelSize: 14
                                color: model.dirty ? "#8a4b00" : (model.hasValue ? "#0f5132" : "#6b7280")
                                verticalAlignment: TextInput.AlignVCenter

                                onEditingFinished: {
                                    if (!enabled) {
                                        return
                                    }

                                    var currentText = root.valueText(model.value, model.valueHex, model.hasValue, model.typeName)
                                    if (text !== currentText) {
                                        root.store.setParameterValueText(model.parameterId, text, "ParameterPage")
                                    }
                                }
                            }
                        }
                    }
                    BodyCell {
                        width: root.columnWidth(3)
                        text: model.hasDefinition ? model.typeName : "-"
                    }
                    BodyCell {
                        width: root.columnWidth(4)
                        text: model.busy ? model.busyReason : (model.lastError.length > 0 ? "Error" : (model.dirty ? "Modified" : "Ready"))
                        textColor: model.busy ? "#1f5fbf" : (model.lastError.length > 0 ? "#b42318" : (model.dirty ? "#8a4b00" : "#4b5563"))
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
