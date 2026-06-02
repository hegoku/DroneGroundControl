import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DroneGroundControl
import "../components"

ScrollView {
    id: root

    property var store: droneSession.parameterStore

    readonly property int pageFontSize: 13
    readonly property int inputFontSize: 11
    readonly property int tableFontSize: 12
    readonly property int tableHeaderFontSize: 12
    readonly property int tableRowHeight: 28
    readonly property int controlHeight: 28
    readonly property int anotcConfigParMagRotation: 16
    readonly property int anotcConfigParMagDeclination: 17
    readonly property int anotcConfigParMagKX1: 18
    readonly property int anotcConfigParMagKX2: 19
    readonly property int anotcConfigParMagKX3: 20
    readonly property int anotcConfigParMagKY1: 21
    readonly property int anotcConfigParMagKY2: 22
    readonly property int anotcConfigParMagKY3: 23
    readonly property int anotcConfigParMagKZ1: 24
    readonly property int anotcConfigParMagKZ2: 25
    readonly property int anotcConfigParMagKZ3: 26
    readonly property int anotcConfigParMagOffsetX: 27
    readonly property int anotcConfigParMagOffsetY: 28
    readonly property int anotcConfigParMagOffsetZ: 29
    readonly property var magnetometerParameterIds: [
        anotcConfigParMagRotation,
        anotcConfigParMagDeclination,
        anotcConfigParMagKX1,
        anotcConfigParMagKX2,
        anotcConfigParMagKX3,
        anotcConfigParMagKY1,
        anotcConfigParMagKY2,
        anotcConfigParMagKY3,
        anotcConfigParMagKZ1,
        anotcConfigParMagKZ2,
        anotcConfigParMagKZ3,
        anotcConfigParMagOffsetX,
        anotcConfigParMagOffsetY,
        anotcConfigParMagOffsetZ
    ]
    readonly property var matrixParameterIds: [
        anotcConfigParMagKX1,
        anotcConfigParMagKX2,
        anotcConfigParMagKX3,
        anotcConfigParMagKY1,
        anotcConfigParMagKY2,
        anotcConfigParMagKY3,
        anotcConfigParMagKZ1,
        anotcConfigParMagKZ2,
        anotcConfigParMagKZ3
    ]
    readonly property var offsetParameterIds: [
        anotcConfigParMagOffsetX,
        anotcConfigParMagOffsetY,
        anotcConfigParMagOffsetZ
    ]

    property int rotationIndex: 0
    property int loadedRotationIndex: 0
    property string declinationText: ""
    property string loadedDeclinationText: ""
    property string referenceFieldText: ""
    property var matrixValues: ["-", "-", "-", "-", "-", "-", "-", "-", "-"]
    property var offsetValues: ["-", "-", "-"]
    property bool loadingParameters: false
    property bool savingSettings: false
    property bool savingCalibration: false
    property bool calibrationDirty: false
    property var pointCloudWindow: null
    property int loadGeneration: 0
    property string statusMessage: droneSession.isOpen
                                   ? "Waiting for drone ready state"
                                   : "Connect to a drone to load magnetometer settings"

    readonly property bool flightReady: flight.status === Flight.FLIGHT_STATUS_READY
    readonly property bool busy: loadingParameters
                                 || savingSettings
                                 || savingCalibration
                                 || magnetometerCalibration.collecting
    readonly property bool controlsEnabled: droneSession.isOpen && flightReady && !busy

    function clampIndex(value, maximum) {
        var number = Number(value)
        if (!isFinite(number)) {
            return 0
        }
        return Math.max(0, Math.min(maximum, Math.floor(number)))
    }

    function parameterValue(parameterId) {
        var parameter = store.parameter(parameterId)
        return parameter && parameter.hasValue ? parameter.value : undefined
    }

    function parameterText(parameterId, fallback) {
        var value = parameterValue(parameterId)
        if (value === undefined || value === null || String(value).length === 0) {
            return fallback
        }
        if (typeof value === "number") {
            return formatNumber(value)
        }
        return String(value)
    }

    function formatNumber(value) {
        var number = Number(value)
        if (!isFinite(number)) {
            return "-"
        }
        return String(Number(number.toFixed(6)))
    }

    function settingsChanged() {
        return rotationIndex !== loadedRotationIndex
            || String(declinationText).trim() !== loadedDeclinationText
    }

    function restoreLoadedSettings() {
        rotationIndex = loadedRotationIndex
        declinationText = loadedDeclinationText
    }

    function applyLoadedParameters() {
        loadedRotationIndex = clampIndex(parameterValue(anotcConfigParMagRotation), 7)
        loadedDeclinationText = parameterText(anotcConfigParMagDeclination, "")
        restoreLoadedSettings()

        var matrix = []
        for (var matrixIndex = 0; matrixIndex < matrixParameterIds.length; ++matrixIndex) {
            matrix.push(parameterText(matrixParameterIds[matrixIndex], "-"))
        }
        matrixValues = matrix

        var offsets = []
        for (var offsetIndex = 0; offsetIndex < offsetParameterIds.length; ++offsetIndex) {
            offsets.push(parameterText(offsetParameterIds[offsetIndex], "-"))
        }
        offsetValues = offsets
    }

    function applyCalculatedCalibration() {
        if (!magnetometerCalibration.calibrationValid) {
            return
        }

        var matrix = []
        var computedMatrix = magnetometerCalibration.softIronMatrix
        for (var matrixIndex = 0; matrixIndex < computedMatrix.length; ++matrixIndex) {
            matrix.push(formatNumber(computedMatrix[matrixIndex]))
        }
        matrixValues = matrix

        var offsets = []
        var computedOffsets = magnetometerCalibration.hardIronOffsets
        for (var offsetIndex = 0; offsetIndex < computedOffsets.length; ++offsetIndex) {
            offsets.push(formatNumber(computedOffsets[offsetIndex]))
        }
        offsetValues = offsets
        calibrationDirty = true
        statusMessage = "Calibration calculated with "
                      + formatNumber(magnetometerCalibration.residualRmsPercent)
                      + "% RMS error"
    }

    function refreshMagnetometerParameters() {
        if (busy) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone to load magnetometer settings"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before loading magnetometer settings"
            return
        }

        loadingParameters = true
        loadGeneration += 1
        var generation = loadGeneration
        statusMessage = "Loading magnetometer settings"
        refreshParameterAt(0, generation)
    }

    function refreshParameterAt(index, generation) {
        if (generation !== loadGeneration) {
            return
        }
        if (index >= magnetometerParameterIds.length) {
            loadingParameters = false
            calibrationDirty = false
            applyLoadedParameters()
            statusMessage = "Magnetometer settings loaded"
            return
        }

        var parameterId = magnetometerParameterIds[index]
        store.refreshParameterInfo(parameterId, function() {
            store.refreshParameterValue(parameterId, function() {
                refreshParameterAt(index + 1, generation)
            }, function(error) {
                finishLoadFailure(parameterId, error.reason, generation)
            }, "SensorMagnetometerPage")
        }, function(error) {
            finishLoadFailure(parameterId, error.reason, generation)
        }, "SensorMagnetometerPage")
    }

    function finishLoadFailure(parameterId, reason, generation) {
        if (generation !== loadGeneration) {
            return
        }
        loadingParameters = false
        statusMessage = "Parameter " + parameterId + " load failed: " + reason
    }

    function declinationIsValid() {
        var value = Number(String(declinationText).trim())
        return isFinite(value) && value >= -180 && value <= 180
    }

    function requestedFieldStrength() {
        var normalized = String(referenceFieldText).trim()
        if (normalized.length === 0) {
            return 0
        }
        var value = Number(normalized)
        return isFinite(value) && value > 0 ? value : -1
    }

    function cancelSettings() {
        if (busy) {
            return
        }
        restoreLoadedSettings()
        statusMessage = "Magnetometer settings restored"
    }

    function saveSettings() {
        if (!controlsEnabled) {
            return
        }
        if (!declinationIsValid()) {
            statusMessage = "Declination must be between -180 and 180 degrees"
            return
        }

        savingSettings = true
        statusMessage = "Writing magnetometer settings"
        var writes = [
            { "id": anotcConfigParMagRotation, "value": String(rotationIndex) },
            { "id": anotcConfigParMagDeclination, "value": String(declinationText).trim() }
        ]
        writeParameterAt(0, writes, function() {
            persistWrites("Magnetometer settings saved")
        }, function(message) {
            finishSaveFailure("Magnetometer settings save failed: " + message)
        })
    }

    function startCollection() {
        if (!controlsEnabled) {
            return
        }
        calibrationDirty = false
        magnetometerCalibration.startCollection()
        statusMessage = "Collecting raw samples. Rotate the aircraft through every orientation."
    }

    function stopCollection() {
        magnetometerCalibration.stopCollection()
        statusMessage = "Collection stopped with " + magnetometerCalibration.sampleCount + " samples"
    }

    function calculateCalibration() {
        if (!controlsEnabled) {
            return
        }
        var fieldStrength = requestedFieldStrength()
        if (fieldStrength < 0) {
            statusMessage = "Reference field norm must be a positive number or left blank for auto"
            return
        }

        statusMessage = "Calculating magnetometer calibration"
        if (!magnetometerCalibration.calculate(fieldStrength)) {
            statusMessage = "Calibration failed: " + magnetometerCalibration.lastError
        }
    }

    function saveCalibration() {
        if (!controlsEnabled || !calibrationDirty || !magnetometerCalibration.calibrationValid) {
            return
        }

        savingCalibration = true
        statusMessage = "Writing magnetometer calibration"
        var writes = []
        var matrix = magnetometerCalibration.softIronMatrix
        var offsets = magnetometerCalibration.hardIronOffsets
        for (var matrixIndex = 0; matrixIndex < matrixParameterIds.length; ++matrixIndex) {
            writes.push({ "id": matrixParameterIds[matrixIndex], "value": String(matrix[matrixIndex]) })
        }
        for (var offsetIndex = 0; offsetIndex < offsetParameterIds.length; ++offsetIndex) {
            writes.push({ "id": offsetParameterIds[offsetIndex], "value": String(offsets[offsetIndex]) })
        }

        writeParameterAt(0, writes, function() {
            persistWrites("Magnetometer calibration saved")
        }, function(message) {
            finishSaveFailure("Magnetometer calibration save failed: " + message)
        })
    }

    function openPointCloudWindow() {
        if (!pointCloudWindow) {
            pointCloudWindow = pointCloudWindowComponent.createObject(root)
        }
        pointCloudWindow.show()
        pointCloudWindow.raise()
        pointCloudWindow.requestActivate()
    }

    function writeParameterAt(index, writes, onComplete, onFailure) {
        if (index >= writes.length) {
            onComplete()
            return
        }

        var item = writes[index]
        store.refreshParameterInfo(item.id, function() {
            if (!store.setParameterValueText(item.id, item.value, "SensorMagnetometerPage")) {
                onFailure("parameter " + item.id + " rejected value")
                return
            }
            store.writeParameter(item.id, function() {
                writeParameterAt(index + 1, writes, onComplete, onFailure)
            }, function(error) {
                onFailure("parameter " + item.id + " write failed: " + error.reason)
            }, "SensorMagnetometerPage")
        }, function(error) {
            onFailure("parameter " + item.id + " info failed: " + error.reason)
        }, "SensorMagnetometerPage")
    }

    function persistWrites(successMessage) {
        droneSession.saveParameters(function() {
            savingSettings = false
            savingCalibration = false
            calibrationDirty = false
            statusMessage = successMessage
            refreshMagnetometerParameters()
        }, function(error) {
            finishSaveFailure("persist failed: " + error.reason)
        })
    }

    function finishSaveFailure(message) {
        savingSettings = false
        savingCalibration = false
        statusMessage = message
    }

    Component.onCompleted: refreshMagnetometerParameters()

    Component {
        id: pointCloudWindowComponent

        MagnetometerPointCloudWindow {
            rawPoints: magnetometerCalibration.rawPointCloud
            calibratedPoints: magnetometerCalibration.calibratedPointCloud
        }
    }

    Connections {
        target: droneSession

        function onIsOpenChanged() {
            if (droneSession.isOpen) {
                root.refreshMagnetometerParameters()
            } else {
                root.loadGeneration += 1
                root.loadingParameters = false
                root.savingSettings = false
                root.savingCalibration = false
                root.calibrationDirty = false
                magnetometerCalibration.stopCollection()
                magnetometerCalibration.clearSamples()
                root.statusMessage = "Connect to a drone to load magnetometer settings"
            }
        }
    }

    Connections {
        target: flight

        function onStatusChanged() {
            if (droneSession.isOpen && root.flightReady) {
                root.refreshMagnetometerParameters()
            }
        }
    }

    Connections {
        target: magnetometerCalibration

        function onCalibrationChanged() {
            root.applyCalculatedCalibration()
        }
    }

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: root.availableWidth
        spacing: 12

        StyledPanel {
            title: "Settings"
            Layout.fillWidth: true
            Layout.preferredHeight: 184

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                SettingRow {
                    label: "Rotation"
                    options: ["0\u00b0", "YAW_45\u00b0", "YAW_90\u00b0", "YAW_135\u00b0",
                              "YAW_180\u00b0", "YAW_225\u00b0", "YAW_270\u00b0", "YAW_315\u00b0"]
                    selectedIndex: root.rotationIndex
                    enabled: root.controlsEnabled

                    onActivated: function(index) {
                        root.rotationIndex = index
                    }
                }

                NumberSettingRow {
                    label: "Magnetic Declination"
                    valueText: root.declinationText
                    enabled: root.controlsEnabled

                    onEdited: function(text) {
                        root.declinationText = text
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#e1e5eb"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Item { Layout.fillWidth: true }

                    StyledButton {
                        text: "Cancel"
                        styleName: "cancel-button"
                        enabled: root.controlsEnabled && root.settingsChanged()
                        Layout.preferredWidth: 88
                        onClicked: root.cancelSettings()
                    }

                    StyledButton {
                        text: root.savingSettings ? "Saving..." : "Save"
                        styleName: "primary-button"
                        enabled: root.controlsEnabled
                        Layout.preferredWidth: 88
                        onClicked: root.saveSettings()
                    }
                }
            }
        }

        StyledPanel {
            title: "Magnetometer Calibration"
            Layout.fillWidth: true
            Layout.preferredHeight: 424

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Text {
                    Layout.fillWidth: true
                    text: "Rotate the aircraft slowly through all orientations. The fit calculates hard-iron offsets and a full 3x3 soft-iron correction matrix."
                    color: "#59616d"
                    font.pixelSize: root.inputFontSize
                    wrapMode: Text.WordWrap
                }

                MatrixTable {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 860
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 860
                    spacing: 12

                    Text {
                        text: "Reference field norm"
                        color: "#2f343b"
                        font.pixelSize: root.pageFontSize
                        Layout.preferredWidth: 150
                    }

                    Rectangle {
                        Layout.preferredWidth: 150
                        Layout.preferredHeight: root.controlHeight
                        color: referenceFieldEditor.enabled ? "#ffffff" : "#fbfcfd"
                        border.color: referenceFieldEditor.enabled && referenceFieldEditor.activeFocus ? "#4c75f2" : "#cfd5dd"
                        border.width: 1

                        TextInput {
                            id: referenceFieldEditor
                            anchors.fill: parent
                            anchors.margins: 4
                            text: root.referenceFieldText
                            enabled: root.controlsEnabled
                            selectByMouse: true
                            font.pixelSize: root.inputFontSize
                            color: enabled ? "#2d333b" : "#9aa3af"
                            verticalAlignment: TextInput.AlignVCenter
                            validator: DoubleValidator {
                                bottom: 0
                                notation: DoubleValidator.StandardNotation
                            }
                            onTextEdited: root.referenceFieldText = text
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "Optional. Leave blank to preserve the sensor's average scale."
                        color: "#7b8491"
                        font.pixelSize: root.inputFontSize
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 860
                    spacing: 12

                    StyledProgressBar {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 7
                        from: 0
                        to: 100
                        value: magnetometerCalibration.coveragePercent
                        progressColor: "#2458f2"
                        enabled: droneSession.isOpen
                    }

                    Text {
                        text: magnetometerCalibration.coveragePercent + "% coverage, "
                              + magnetometerCalibration.sampleCount + " samples"
                        color: "#59616d"
                        font.pixelSize: root.inputFontSize
                        Layout.preferredWidth: 180
                        horizontalAlignment: Text.AlignRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 860
                    spacing: 10

                    StyledButton {
                        text: magnetometerCalibration.collecting ? "Stop Collection" : "Start Collection"
                        styleName: magnetometerCalibration.collecting ? "cancel-button" : "primary-button"
                        enabled: magnetometerCalibration.collecting || root.controlsEnabled
                        Layout.fillWidth: true
                        onClicked: {
                            if (magnetometerCalibration.collecting) {
                                root.stopCollection()
                            } else {
                                root.startCollection()
                            }
                        }
                    }

                    StyledButton {
                        text: "Calculate"
                        styleName: "cancel-button"
                        enabled: root.controlsEnabled && magnetometerCalibration.sampleCount >= 100
                        Layout.fillWidth: true
                        onClicked: root.calculateCalibration()
                    }

                    StyledButton {
                        text: root.savingCalibration ? "Saving..." : "Save Calibration"
                        styleName: "primary-button"
                        enabled: root.controlsEnabled
                                 && root.calibrationDirty
                                 && magnetometerCalibration.calibrationValid
                        Layout.fillWidth: true
                        onClicked: root.saveCalibration()
                    }

                    StyledButton {
                        text: "Open 3D View"
                        styleName: "cancel-button"
                        Layout.fillWidth: true
                        onClicked: root.openPointCloudWindow()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: magnetometerCalibration.calibrationValid
                          ? "Fit target: " + root.formatNumber(magnetometerCalibration.targetFieldStrength)
                            + ", RMS error: " + root.formatNumber(magnetometerCalibration.residualRmsPercent) + "%"
                          : root.statusMessage
                    color: magnetometerCalibration.lastError.length > 0 ? "#c43d3d" : "#59616d"
                    font.pixelSize: root.inputFontSize
                    wrapMode: Text.WordWrap
                }
            }
        }

    }

    component SettingRow: RowLayout {
        id: settingRow

        property string label: ""
        property var options: []
        property int selectedIndex: 0
        property bool enabled: true
        signal activated(int index)

        onSelectedIndexChanged: selector.currentIndex = selectedIndex

        Layout.fillWidth: true
        spacing: 20

        Text {
            text: settingRow.label
            color: "#2f343b"
            font.pixelSize: root.pageFontSize
            Layout.preferredWidth: 210
            Layout.alignment: Qt.AlignVCenter
        }

        StyledSelect {
            id: selector

            model: settingRow.options
            currentIndex: settingRow.selectedIndex
            enabled: settingRow.enabled
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            Layout.preferredHeight: root.controlHeight

            onActivated: function(index) {
                settingRow.activated(index)
            }
        }
    }

    component NumberSettingRow: RowLayout {
        id: settingRow

        property string label: ""
        property string valueText: ""
        property bool enabled: true
        signal edited(string text)

        Layout.fillWidth: true
        spacing: 20

        Text {
            text: settingRow.label
            color: "#2f343b"
            font.pixelSize: root.pageFontSize
            Layout.preferredWidth: 210
            Layout.alignment: Qt.AlignVCenter
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            Layout.preferredHeight: root.controlHeight
            color: editor.enabled ? "#ffffff" : "#fbfcfd"
            border.color: editor.enabled && editor.activeFocus ? "#4c75f2" : "#cfd5dd"
            border.width: 1

            TextInput {
                id: editor

                anchors.fill: parent
                anchors.margins: 4
                text: settingRow.valueText
                enabled: settingRow.enabled
                selectByMouse: true
                font.pixelSize: root.inputFontSize
                color: enabled ? "#2d333b" : "#9aa3af"
                verticalAlignment: TextInput.AlignVCenter
                validator: DoubleValidator {
                    bottom: -180
                    top: 180
                    notation: DoubleValidator.StandardNotation
                }
                onTextEdited: settingRow.edited(text)
            }
        }
    }

    component MatrixTable: Rectangle {
        implicitHeight: root.tableRowHeight * 4
        radius: 4
        color: "#ffffff"
        border.color: "#d8dde4"
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            MatrixRow {
                axis: ""
                xValue: "Raw X"
                yValue: "Raw Y"
                zValue: "Raw Z"
                offset: "Offset"
                header: true
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }
            MatrixRow {
                axis: "Corrected X"
                xValue: root.matrixValues[0]
                yValue: root.matrixValues[1]
                zValue: root.matrixValues[2]
                offset: root.offsetValues[0]
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }
            MatrixRow {
                axis: "Corrected Y"
                xValue: root.matrixValues[3]
                yValue: root.matrixValues[4]
                zValue: root.matrixValues[5]
                offset: root.offsetValues[1]
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }
            MatrixRow {
                axis: "Corrected Z"
                xValue: root.matrixValues[6]
                yValue: root.matrixValues[7]
                zValue: root.matrixValues[8]
                offset: root.offsetValues[2]
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }
        }
    }

    component MatrixRow: Item {
        id: tableRow

        property string axis: ""
        property string xValue: ""
        property string yValue: ""
        property string zValue: ""
        property string offset: ""
        property bool header: false

        RowLayout {
            anchors.fill: parent
            spacing: 0

            TableCell { text: tableRow.axis; header: tableRow.header; Layout.preferredWidth: 116; Layout.fillHeight: true }
            TableCell { text: tableRow.xValue; header: tableRow.header; Layout.fillWidth: true; Layout.fillHeight: true }
            TableCell { text: tableRow.yValue; header: tableRow.header; Layout.fillWidth: true; Layout.fillHeight: true }
            TableCell { text: tableRow.zValue; header: tableRow.header; Layout.fillWidth: true; Layout.fillHeight: true }
            TableCell { text: tableRow.offset; header: tableRow.header; showDivider: false; Layout.fillWidth: true; Layout.fillHeight: true }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#d8dde4"
            visible: tableRow.header || tableRow.axis !== "Corrected Z"
        }
    }

    component TableCell: Item {
        id: tableCell

        property string text: ""
        property bool header: false
        property bool showDivider: true

        Text {
            anchors.fill: parent
            anchors.margins: 4
            text: tableCell.text
            color: "#252b33"
            font.pixelSize: tableCell.header ? root.tableHeaderFontSize : root.tableFontSize
            font.bold: tableCell.header
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: "#d8dde4"
            visible: tableCell.showDivider
        }
    }
}
