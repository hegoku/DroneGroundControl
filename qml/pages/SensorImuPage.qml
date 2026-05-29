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
    readonly property int tableFontSize: 13
    readonly property int tableHeaderFontSize: 13
    readonly property int tableRowHeight: 28
    readonly property int controlHeight: 28
    readonly property int anotcConfigParImuRotation: 3
    readonly property int anotcConfigParGyroAutoCal: 4
    readonly property int anotcConfigParGyroCutoff: 5
    readonly property int anotcConfigParAccelCutoff: 6
    readonly property int anotcConfigParAccelKX: 7
    readonly property int anotcConfigParAccelKY: 8
    readonly property int anotcConfigParAccelKZ: 9
    readonly property int anotcConfigParAccelOffsetX: 10
    readonly property int anotcConfigParAccelOffsetY: 11
    readonly property int anotcConfigParAccelOffsetZ: 12
    readonly property int anotcConfigParGyroOffsetX: 13
    readonly property int anotcConfigParGyroOffsetY: 14
    readonly property int anotcConfigParGyroOffsetZ: 15
    readonly property var accelDirections: [
        { "label": "Up", "direction": connectionSessionBridge.accelDirectionUp },
        { "label": "Forward", "direction": connectionSessionBridge.accelDirectionForward },
        { "label": "Left", "direction": connectionSessionBridge.accelDirectionLeft },
        { "label": "Down", "direction": connectionSessionBridge.accelDirectionDown },
        { "label": "Backward", "direction": connectionSessionBridge.accelDirectionBackward },
        { "label": "Right", "direction": connectionSessionBridge.accelDirectionRight }
    ]
    readonly property var imuParameterIds: [
        anotcConfigParImuRotation,
        anotcConfigParGyroAutoCal,
        anotcConfigParGyroCutoff,
        anotcConfigParAccelCutoff,
        anotcConfigParAccelKX,
        anotcConfigParAccelKY,
        anotcConfigParAccelKZ,
        anotcConfigParAccelOffsetX,
        anotcConfigParAccelOffsetY,
        anotcConfigParAccelOffsetZ,
        anotcConfigParGyroOffsetX,
        anotcConfigParGyroOffsetY,
        anotcConfigParGyroOffsetZ
    ]

    property int rotationIndex: 0
    property int gyroAutoCalibrationIndex: 0
    property string gyroCutoffText: ""
    property string accelCutoffText: ""
    property int loadedRotationIndex: 0
    property int loadedGyroAutoCalibrationIndex: 0
    property string loadedGyroCutoffText: ""
    property string loadedAccelCutoffText: ""
    property string accelKX: "-"
    property string accelKY: "-"
    property string accelKZ: "-"
    property string accelOffsetX: "-"
    property string accelOffsetY: "-"
    property string accelOffsetZ: "-"
    property string gyroOffsetX: "-"
    property string gyroOffsetY: "-"
    property string gyroOffsetZ: "-"
    property bool loadingParameters: false
    property bool savingParameters: false
    property bool calibratingGyro: false
    property bool refreshingGyroOffsets: false
    property bool calibratingAccel: false
    property bool sendingAccelCalibrationCommand: false
    property bool refreshingAccelCalibration: false
    property int activeAccelDirection: 0
    property int accelProgressUp: 0
    property int accelProgressDown: 0
    property int accelProgressForward: 0
    property int accelProgressBackward: 0
    property int accelProgressLeft: 0
    property int accelProgressRight: 0
    property int gyroCalibrationProgress: 0
    property string statusMessage: droneSession.isOpen ? "Ready" : "Connect to a drone to load IMU settings"
    property int loadGeneration: 0
    readonly property bool flightReady: flight.status === Flight.FLIGHT_STATUS_READY
    readonly property bool controlsEnabled: droneSession.isOpen
                                            && flightReady
                                            && !loadingParameters
                                            && !savingParameters
                                            && !calibratingGyro
                                            && !refreshingGyroOffsets
                                            && !calibratingAccel
                                            && !sendingAccelCalibrationCommand
                                            && !refreshingAccelCalibration
    readonly property bool accelDirectionControlsEnabled: droneSession.isOpen
                                                          && calibratingAccel
                                                          && !loadingParameters
                                                          && !savingParameters
                                                          && !calibratingGyro
                                                          && !refreshingGyroOffsets
                                                          && !sendingAccelCalibrationCommand
                                                          && !refreshingAccelCalibration
                                                          && activeAccelDirection === connectionSessionBridge.accelDirectionStart

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
            return Number.isInteger(value) ? String(value) : String(Number(value.toFixed(6)))
        }
        return String(value)
    }

    function bytesFromHex(hex) {
        var normalized = String(hex || "").trim()
        if (normalized.length === 0) {
            return []
        }

        var parts = normalized.split(/\s+/)
        var bytes = []
        for (var i = 0; i < parts.length; ++i) {
            var value = parseInt(parts[i], 16)
            if (!isFinite(value) || value < 0 || value > 255) {
                return []
            }
            bytes.push(value)
        }
        return bytes
    }

    function handleCommandFramePayload(frame) {
        if (Number(frame.function) !== connectionSessionBridge.frameCmdSend) {
            return
        }

        var bytes = bytesFromHex(frame.payloadHex)
        if (bytes.length < 5) {
            return
        }

        var commandId = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2]
        if (commandId === connectionSessionBridge.commandCalibrateGyro) {
            handleGyroCommandResponse(bytes)
        } else if (commandId === connectionSessionBridge.commandCalibrateAccel) {
            handleAccelCommandResponse(bytes)
        }
    }

    function handleGyroCommandResponse(bytes) {
        if (bytes.length < 5) {
            return
        }

        var code = bytes[3]
        if (code !== 0) {
            calibratingGyro = false
            statusMessage = "Gyro calibration failed with code " + code
            return
        }

        var wasCalibrating = calibratingGyro
        gyroCalibrationProgress = Math.max(0, Math.min(100, bytes[4]))
        statusMessage = "Calibrating gyro " + gyroCalibrationProgress + "%"
        calibratingGyro = gyroCalibrationProgress < 100
        if (!calibratingGyro && wasCalibrating) {
            statusMessage = "Gyro calibration complete"
            refreshGyroOffsets()
        }
    }

    function handleAccelCommandResponse(bytes) {
        if (bytes.length < 6) {
            return
        }

        var code = bytes[3]
        var direction = bytes[4]
        if (code !== 0) {
            sendingAccelCalibrationCommand = false
            activeAccelDirection = connectionSessionBridge.accelDirectionStart
            if (direction === connectionSessionBridge.accelDirectionStart) {
                calibratingAccel = false
            }
            statusMessage = "Accelerometer calibration failed with code " + code
            return
        }

        var progress = Math.max(0, Math.min(100, bytes[5]))
        if (direction === connectionSessionBridge.accelDirectionStart) {
            sendingAccelCalibrationCommand = false
            activeAccelDirection = connectionSessionBridge.accelDirectionStart
            calibratingAccel = false
            statusMessage = "Accelerometer calibration complete"
            refreshAccelCalibrationValues()
            return
        }

        setAccelDirectionProgress(direction, progress)
        statusMessage = progress >= 100
                        ? accelDirectionLabel(direction) + " data collected"
                        : "Collecting " + accelDirectionLabel(direction) + " " + progress + "%"
        if (progress >= 100 && activeAccelDirection === direction) {
            activeAccelDirection = connectionSessionBridge.accelDirectionStart
        }
    }

    function settingsChanged() {
        return rotationIndex !== loadedRotationIndex
            || gyroAutoCalibrationIndex !== loadedGyroAutoCalibrationIndex
            || String(gyroCutoffText).trim() !== loadedGyroCutoffText
            || String(accelCutoffText).trim() !== loadedAccelCutoffText
    }

    function restoreLoadedSettings() {
        rotationIndex = loadedRotationIndex
        gyroAutoCalibrationIndex = loadedGyroAutoCalibrationIndex
        gyroCutoffText = loadedGyroCutoffText
        accelCutoffText = loadedAccelCutoffText
    }

    function applyLoadedParameters() {
        loadedRotationIndex = clampIndex(parameterValue(anotcConfigParImuRotation), 7)
        loadedGyroAutoCalibrationIndex = clampIndex(parameterValue(anotcConfigParGyroAutoCal), 1)
        loadedGyroCutoffText = parameterText(anotcConfigParGyroCutoff, "")
        loadedAccelCutoffText = parameterText(anotcConfigParAccelCutoff, "")
        restoreLoadedSettings()
        accelKX = parameterText(anotcConfigParAccelKX, "-")
        accelKY = parameterText(anotcConfigParAccelKY, "-")
        accelKZ = parameterText(anotcConfigParAccelKZ, "-")
        accelOffsetX = parameterText(anotcConfigParAccelOffsetX, "-")
        accelOffsetY = parameterText(anotcConfigParAccelOffsetY, "-")
        accelOffsetZ = parameterText(anotcConfigParAccelOffsetZ, "-")
        gyroOffsetX = parameterText(anotcConfigParGyroOffsetX, "-")
        gyroOffsetY = parameterText(anotcConfigParGyroOffsetY, "-")
        gyroOffsetZ = parameterText(anotcConfigParGyroOffsetZ, "-")
    }

    function applyGyroOffsets() {
        gyroOffsetX = parameterText(anotcConfigParGyroOffsetX, "-")
        gyroOffsetY = parameterText(anotcConfigParGyroOffsetY, "-")
        gyroOffsetZ = parameterText(anotcConfigParGyroOffsetZ, "-")
    }

    function applyAccelCalibrationValues() {
        accelKX = parameterText(anotcConfigParAccelKX, "-")
        accelKY = parameterText(anotcConfigParAccelKY, "-")
        accelKZ = parameterText(anotcConfigParAccelKZ, "-")
        accelOffsetX = parameterText(anotcConfigParAccelOffsetX, "-")
        accelOffsetY = parameterText(anotcConfigParAccelOffsetY, "-")
        accelOffsetZ = parameterText(anotcConfigParAccelOffsetZ, "-")
    }

    function refreshImuParameters() {
        if (loadingParameters || savingParameters || calibratingGyro || refreshingGyroOffsets
                || calibratingAccel || sendingAccelCalibrationCommand || refreshingAccelCalibration) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone to load IMU settings"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before loading IMU settings"
            return
        }

        loadingParameters = true
        loadGeneration += 1
        var generation = loadGeneration
        statusMessage = "Loading IMU settings"
        refreshParameterAt(0, generation)
    }

    function refreshParameterAt(index, generation) {
        if (generation !== loadGeneration) {
            return
        }
        if (index >= imuParameterIds.length) {
            loadingParameters = false
            applyLoadedParameters()
            statusMessage = "IMU settings loaded"
            return
        }

        var parameterId = imuParameterIds[index]
        store.refreshParameterInfo(parameterId, function() {
            store.refreshParameterValue(parameterId, function() {
                applyLoadedParameters()
                refreshParameterAt(index + 1, generation)
            }, function(error) {
                finishLoadFailure(parameterId, error.reason, generation)
            }, "SensorImuPage")
        }, function(error) {
            finishLoadFailure(parameterId, error.reason, generation)
        }, "SensorImuPage")
    }

    function finishLoadFailure(parameterId, reason, generation) {
        if (generation !== loadGeneration) {
            return
        }
        loadingParameters = false
        statusMessage = "Parameter " + parameterId + " load failed: " + reason
    }

    function refreshGyroOffsets() {
        if (refreshingGyroOffsets || !droneSession.isOpen) {
            return
        }

        refreshingGyroOffsets = true
        statusMessage = "Refreshing gyro offsets"
        refreshGyroOffsetAt(0, [anotcConfigParGyroOffsetX,
                                anotcConfigParGyroOffsetY,
                                anotcConfigParGyroOffsetZ])
    }

    function refreshGyroOffsetAt(index, parameterIds) {
        if (index >= parameterIds.length) {
            refreshingGyroOffsets = false
            applyGyroOffsets()
            statusMessage = "Gyro offsets refreshed"
            return
        }

        var parameterId = parameterIds[index]
        store.refreshParameterInfo(parameterId, function() {
            store.refreshParameterValue(parameterId, function() {
                applyGyroOffsets()
                refreshGyroOffsetAt(index + 1, parameterIds)
            }, function(error) {
                finishGyroOffsetRefreshFailure(parameterId, error.reason)
            }, "SensorImuPage")
        }, function(error) {
            finishGyroOffsetRefreshFailure(parameterId, error.reason)
        }, "SensorImuPage")
    }

    function finishGyroOffsetRefreshFailure(parameterId, reason) {
        refreshingGyroOffsets = false
        statusMessage = "Gyro offset " + parameterId + " refresh failed: " + reason
    }

    function refreshAccelCalibrationValues() {
        if (refreshingAccelCalibration || !droneSession.isOpen) {
            return
        }

        refreshingAccelCalibration = true
        statusMessage = "Refreshing accelerometer calibration"
        refreshAccelCalibrationValueAt(0, [anotcConfigParAccelKX,
                                           anotcConfigParAccelKY,
                                           anotcConfigParAccelKZ,
                                           anotcConfigParAccelOffsetX,
                                           anotcConfigParAccelOffsetY,
                                           anotcConfigParAccelOffsetZ])
    }

    function refreshAccelCalibrationValueAt(index, parameterIds) {
        if (index >= parameterIds.length) {
            refreshingAccelCalibration = false
            applyAccelCalibrationValues()
            statusMessage = "Accelerometer calibration values refreshed"
            return
        }

        var parameterId = parameterIds[index]
        store.refreshParameterInfo(parameterId, function() {
            store.refreshParameterValue(parameterId, function() {
                applyAccelCalibrationValues()
                refreshAccelCalibrationValueAt(index + 1, parameterIds)
            }, function(error) {
                finishAccelCalibrationRefreshFailure(parameterId, error.reason)
            }, "SensorImuPage")
        }, function(error) {
            finishAccelCalibrationRefreshFailure(parameterId, error.reason)
        }, "SensorImuPage")
    }

    function finishAccelCalibrationRefreshFailure(parameterId, reason) {
        refreshingAccelCalibration = false
        statusMessage = "Accelerometer parameter " + parameterId + " refresh failed: " + reason
    }

    function cutoffIsValid(text) {
        var normalized = String(text).trim()
        if (!/^[1-9][0-9]{0,2}$/.test(normalized)) {
            return false
        }
        var value = Number(normalized)
        return isFinite(value) && value > 0 && value < 1000 && Math.floor(value) === value
    }

    function cancelChanges() {
        if (loadingParameters || savingParameters || calibratingGyro || refreshingGyroOffsets
                || calibratingAccel || sendingAccelCalibrationCommand || refreshingAccelCalibration) {
            return
        }
        restoreLoadedSettings()
        statusMessage = droneSession.isOpen ? "IMU settings restored" : "Connect to a drone to load IMU settings"
    }

    function saveSettings() {
        if (loadingParameters || savingParameters || calibratingGyro || refreshingGyroOffsets
                || calibratingAccel || sendingAccelCalibrationCommand || refreshingAccelCalibration) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before saving IMU settings"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before saving IMU settings"
            return
        }
        if (!cutoffIsValid(gyroCutoffText)) {
            statusMessage = "Gyroscope low pass filter must be 1-999"
            return
        }
        if (!cutoffIsValid(accelCutoffText)) {
            statusMessage = "Accelerator low pass filter must be 1-999"
            return
        }

        savingParameters = true
        statusMessage = "Writing IMU settings"
        var writes = [
            { "id": anotcConfigParImuRotation, "value": String(rotationIndex) },
            { "id": anotcConfigParGyroAutoCal, "value": String(gyroAutoCalibrationIndex) },
            { "id": anotcConfigParGyroCutoff, "value": String(gyroCutoffText).trim() },
            { "id": anotcConfigParAccelCutoff, "value": String(accelCutoffText).trim() }
        ]
        writeSettingAt(0, writes)
    }

    function calibrateGyro() {
        if (loadingParameters || savingParameters || calibratingGyro || refreshingGyroOffsets
                || calibratingAccel || sendingAccelCalibrationCommand || refreshingAccelCalibration) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before calibrating gyro"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before calibrating gyro"
            return
        }

        calibratingGyro = true
        gyroCalibrationProgress = 0
        statusMessage = "Starting gyro calibration"
        droneSession.sendAnotcCommand(connectionSessionBridge.commandCalibrateGyroCid0,
                                      connectionSessionBridge.commandCalibrateGyroCid1,
                                      connectionSessionBridge.commandCalibrateGyroCid2,
                                      "", function() {
            statusMessage = "Gyro calibration command sent"
        }, function(error) {
            calibratingGyro = false
            statusMessage = "Gyro calibration failed: " + error.reason
        })
    }

    function startAccelCalibration() {
        if (!controlsEnabled) {
            if (droneSession.isOpen && !flightReady) {
                statusMessage = "Drone must be ready before calibrating accelerometer"
            }
            return
        }

        resetAccelProgress()
        calibratingAccel = true
        sendAccelCalibrationCommand(connectionSessionBridge.accelDirectionStart,
                                    "Accelerometer calibration started",
                                    function() {
                                        calibratingAccel = false
                                    })
    }

    function calibrateAccelDirection(direction) {
        if (!accelDirectionControlsEnabled) {
            return
        }

        activeAccelDirection = direction
        setAccelDirectionProgress(direction, 0)
        sendAccelCalibrationCommand(direction,
                                    "Collecting " + accelDirectionLabel(direction),
                                    function() {
                                        activeAccelDirection = connectionSessionBridge.accelDirectionStart
                                    })
    }

    function sendAccelCalibrationCommand(direction, successMessage, rollback) {
        sendingAccelCalibrationCommand = true
        statusMessage = "Sending accelerometer calibration command"
        droneSession.sendAnotcCommand(connectionSessionBridge.commandCalibrateAccelCid0,
                                      connectionSessionBridge.commandCalibrateAccelCid1,
                                      connectionSessionBridge.commandCalibrateAccelCid2,
                                      byteToHex(direction),
                                      function() {
            sendingAccelCalibrationCommand = false
            statusMessage = successMessage
        }, function(error) {
            sendingAccelCalibrationCommand = false
            if (rollback) {
                rollback()
            }
            statusMessage = "Accelerometer calibration failed: " + error.reason
        })
    }

    function byteToHex(value) {
        var hex = (Number(value) & 0xff).toString(16).toUpperCase()
        return hex.length < 2 ? "0" + hex : hex
    }

    function resetAccelProgress() {
        accelProgressUp = 0
        accelProgressDown = 0
        accelProgressForward = 0
        accelProgressBackward = 0
        accelProgressLeft = 0
        accelProgressRight = 0
        activeAccelDirection = connectionSessionBridge.accelDirectionStart
    }

    function accelDirectionLabel(direction) {
        switch (direction) {
        case connectionSessionBridge.accelDirectionUp:
            return "Up"
        case connectionSessionBridge.accelDirectionDown:
            return "Down"
        case connectionSessionBridge.accelDirectionForward:
            return "Forward"
        case connectionSessionBridge.accelDirectionBackward:
            return "Backward"
        case connectionSessionBridge.accelDirectionLeft:
            return "Left"
        case connectionSessionBridge.accelDirectionRight:
            return "Right"
        default:
            return "Accelerometer"
        }
    }

    function accelDirectionProgress(direction) {
        switch (direction) {
        case connectionSessionBridge.accelDirectionUp:
            return accelProgressUp
        case connectionSessionBridge.accelDirectionDown:
            return accelProgressDown
        case connectionSessionBridge.accelDirectionForward:
            return accelProgressForward
        case connectionSessionBridge.accelDirectionBackward:
            return accelProgressBackward
        case connectionSessionBridge.accelDirectionLeft:
            return accelProgressLeft
        case connectionSessionBridge.accelDirectionRight:
            return accelProgressRight
        default:
            return 0
        }
    }

    function setAccelDirectionProgress(direction, progress) {
        switch (direction) {
        case connectionSessionBridge.accelDirectionUp:
            accelProgressUp = progress
            break
        case connectionSessionBridge.accelDirectionDown:
            accelProgressDown = progress
            break
        case connectionSessionBridge.accelDirectionForward:
            accelProgressForward = progress
            break
        case connectionSessionBridge.accelDirectionBackward:
            accelProgressBackward = progress
            break
        case connectionSessionBridge.accelDirectionLeft:
            accelProgressLeft = progress
            break
        case connectionSessionBridge.accelDirectionRight:
            accelProgressRight = progress
            break
        default:
            break
        }
    }

    function writeSettingAt(index, writes) {
        if (index >= writes.length) {
            statusMessage = "Persisting IMU settings"
            droneSession.saveParameters(function() {
                savingParameters = false
                statusMessage = "IMU settings saved"
                refreshImuParameters()
            }, function(error) {
                finishSaveFailure("persist failed: " + error.reason)
            })
            return
        }

        var item = writes[index]
        store.refreshParameterInfo(item.id, function() {
            if (!store.setParameterValueText(item.id, item.value, "SensorImuPage")) {
                finishSaveFailure("parameter " + item.id + " rejected value")
                return
            }
            store.writeParameter(item.id, function() {
                writeSettingAt(index + 1, writes)
            }, function(error) {
                finishSaveFailure("parameter " + item.id + " write failed: " + error.reason)
            }, "SensorImuPage")
        }, function(error) {
            finishSaveFailure("parameter " + item.id + " info failed: " + error.reason)
        }, "SensorImuPage")
    }

    function finishSaveFailure(message) {
        savingParameters = false
        statusMessage = "IMU settings save failed: " + message
    }

    Component.onCompleted: refreshImuParameters()

    Connections {
        target: droneSession

        function onIsOpenChanged() {
            if (droneSession.isOpen) {
                refreshImuParameters()
            } else {
                root.loadGeneration += 1
                root.loadingParameters = false
                root.savingParameters = false
                root.calibratingGyro = false
                root.refreshingGyroOffsets = false
                root.calibratingAccel = false
                root.sendingAccelCalibrationCommand = false
                root.refreshingAccelCalibration = false
                root.resetAccelProgress()
                root.gyroCalibrationProgress = 0
                root.statusMessage = "Connect to a drone to load IMU settings"
            }
        }

        function onCommandFramesReceived(frames) {
            var payloads = connectionSessionBridge.commandFramePayloads(frames)
            for (var i = 0; i < payloads.length; ++i) {
                root.handleCommandFramePayload(payloads[i])
            }
        }
    }

    Connections {
        target: flight

        function onStatusChanged() {
            if (droneSession.isOpen && root.flightReady) {
                root.refreshImuParameters()
            }
        }
    }

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: root.availableWidth
        spacing: 8

        StyledPanel {
            title: "Settings"
            Layout.fillWidth: true
            Layout.preferredHeight: 266

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                SettingRow {
                    label: "Rotation"
                    options: ["0˚", "YAW_45˚", "YAW_90˚", "YAW_135˚",
                              "YAW_180˚", "YAW_225˚", "YAW_270˚", "YAW_315˚"]
                    selectedIndex: root.rotationIndex
                    enabled: root.controlsEnabled

                    onActivated: function(index) {
                        root.rotationIndex = index
                    }
                }

                SettingRow {
                    label: "Gyro Auto Calibration"
                    options: ["Disabled", "Enabled"]
                    selectedIndex: root.gyroAutoCalibrationIndex
                    enabled: root.controlsEnabled

                    onActivated: function(index) {
                        root.gyroAutoCalibrationIndex = index
                    }
                }

                IntegerSettingRow {
                    label: "Gyroscope Low Pass Filter"
                    valueText: root.gyroCutoffText
                    enabled: root.controlsEnabled

                    onEdited: function(text) {
                        root.gyroCutoffText = text
                    }
                }

                IntegerSettingRow {
                    label: "Accelerator Low Pass Filter"
                    valueText: root.accelCutoffText
                    enabled: root.controlsEnabled

                    onEdited: function(text) {
                        root.accelCutoffText = text
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
                        onClicked: root.cancelChanges()
                    }

                    StyledButton {
                        text: root.savingParameters ? "Saving..." : "Save"
                        styleName: "primary-button"
                        enabled: root.controlsEnabled
                        Layout.preferredWidth: 88
                        onClicked: root.saveSettings()
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.availableWidth >= 980 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 12

            StyledPanel {
                title: "Gyroscope Calibration"
                Layout.fillWidth: true
                Layout.preferredWidth: 520
                Layout.preferredHeight: 210
                Layout.alignment: Qt.AlignTop

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10

                    CalibrationTable {
                        showScale: false
                        scaleX: "-"
                        scaleY: "-"
                        scaleZ: "-"
                        offsetX: root.gyroOffsetX
                        offsetY: root.gyroOffsetY
                        offsetZ: root.gyroOffsetZ
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        text: root.calibratingGyro ? root.gyroCalibrationProgress + "%" : "Calibrate"
                        styleName: "primary-button"
                        enabled: root.controlsEnabled || root.calibratingGyro
                        interactive: root.controlsEnabled
                        progressEnabled: root.calibratingGyro
                        progressFrom: 0
                        progressTo: 100
                        progressValue: root.gyroCalibrationProgress
                        Layout.fillWidth: true
                        Layout.leftMargin: 18
                        Layout.rightMargin: 18
                        Layout.preferredHeight: 24
                        onClicked: root.calibrateGyro()
                    }
                }
            }

            StyledPanel {
                title: "Accelerometer Calibration"
                Layout.fillWidth: true
                Layout.preferredWidth: 620
                Layout.preferredHeight: 354
                Layout.minimumHeight: 354
                Layout.alignment: Qt.AlignTop

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    CalibrationTable {
                        scaleX: root.accelKX
                        scaleY: root.accelKY
                        scaleZ: root.accelKZ
                        offsetX: root.accelOffsetX
                        offsetY: root.accelOffsetY
                        offsetZ: root.accelOffsetZ
                        Layout.fillWidth: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: 24
                        rowSpacing: 10

                        Repeater {
                            model: root.accelDirections

                            delegate: ColumnLayout {
                                id: directionControl

                                required property var modelData

                                Layout.fillWidth: true
                                spacing: 6

                                StyledProgressBar {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 6
                                    from: 0
                                    to: 100
                                    value: root.accelDirectionProgress(directionControl.modelData.direction)
                                    progressColor: "#2458f2"
                                    enabled: droneSession.isOpen && root.calibratingAccel
                                }

                                StyledButton {
                                    text: directionControl.modelData.label
                                    styleName: "cancel-button"
                                    enabled: root.accelDirectionControlsEnabled
                                    Layout.fillWidth: true
                                    onClicked: root.calibrateAccelDirection(directionControl.modelData.direction)
                                }
                            }
                        }
                    }

                    StyledButton {
                        text: root.calibratingAccel ? "Calibrating" : "Calibrate"
                        styleName: "primary-button"
                        enabled: root.controlsEnabled
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24
                        onClicked: root.startAccelCalibration()
                    }
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

    component IntegerSettingRow: RowLayout {
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
            id: editorFrame
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            Layout.preferredHeight: root.controlHeight
            radius: 0
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
                validator: IntValidator {
                    bottom: 1
                    top: 999
                }

                onTextEdited: settingRow.edited(text)
            }
        }
    }

    component CalibrationTable: Rectangle {
        id: calibrationTable

        property bool showScale: true
        property string scaleX: "-"
        property string scaleY: "-"
        property string scaleZ: "-"
        property string offsetX: "-"
        property string offsetY: "-"
        property string offsetZ: "-"

        implicitHeight: root.tableRowHeight * 4
        radius: 4
        color: "#ffffff"
        border.color: "#d8dde4"
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TableRow {
                axis: ""
                scale: "Scale"
                offset: "Offset"
                header: true
                showScale: calibrationTable.showScale
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }

            TableRow {
                axis: "X"
                scale: calibrationTable.scaleX
                offset: calibrationTable.offsetX
                showScale: calibrationTable.showScale
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }

            TableRow {
                axis: "Y"
                scale: calibrationTable.scaleY
                offset: calibrationTable.offsetY
                showScale: calibrationTable.showScale
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }

            TableRow {
                axis: "Z"
                scale: calibrationTable.scaleZ
                offset: calibrationTable.offsetZ
                showScale: calibrationTable.showScale
                Layout.fillWidth: true
                Layout.preferredHeight: root.tableRowHeight
            }
        }
    }

    component TableRow: Item {
        id: tableRow

        property string axis: ""
        property string scale: ""
        property string offset: ""
        property bool header: false
        property bool showScale: true

        RowLayout {
            anchors.fill: parent
            spacing: 0

            TableCell {
                text: tableRow.axis
                header: tableRow.header
                Layout.preferredWidth: 90
                Layout.fillHeight: true
            }

            TableCell {
                text: tableRow.scale
                header: tableRow.header
                visible: tableRow.showScale
                Layout.fillWidth: tableRow.showScale
                Layout.fillHeight: true
            }

            TableCell {
                text: tableRow.offset
                header: tableRow.header
                showDivider: false
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: "#d8dde4"
            visible: tableRow.header || tableRow.axis !== "Z"
        }
    }

    component TableCell: Item {
        id: tableCell

        property string text: ""
        property bool header: false
        property bool showDivider: true

        Text {
            anchors.centerIn: parent
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
