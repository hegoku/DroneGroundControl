import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DroneGroundControl
import "../components"

ScrollView {
    id: root

    property var store: droneSession.parameterStore

    readonly property int pageFontSize: 13
    readonly property int controlHeight: 28
    readonly property int escProtocolParameterId: 30
    readonly property int motorMin: 1000
    readonly property int motorMax: 2000
    readonly property var protocolOptions: ["Brushed", "DSHOT300"]

    property int loadedProtocol: 0
    property int selectedProtocol: 0
    property bool loadingProtocol: false
    property bool savingProtocol: false
    property bool changingMotorTestMode: false
    property bool throttleRequestActive: false
    property bool throttleRequestPending: false
    property bool motorTestMode: false
    property var motorValues: [1000, 1000, 1000, 1000]
    property var liveMotorValues: [0, 0, 0, 0]
    property string statusMessage: droneSession.isOpen ? "Ready" : "Connect to a drone to edit mixer settings"

    readonly property bool flightReady: flight.status === Flight.FLIGHT_STATUS_READY
    readonly property bool motorTestStatus: flight.status === Flight.FLIGHT_STATUS_MOTOR_TEST
    readonly property bool protocolControlsEnabled: droneSession.isOpen && flightReady && !loadingProtocol && !savingProtocol
    readonly property bool motorTestModeToggleEnabled: droneSession.isOpen && (flightReady || motorTestStatus || motorTestMode) && !changingMotorTestMode
    readonly property bool motorControlsEnabled: droneSession.isOpen && motorTestMode && !changingMotorTestMode

    function parseProtocol(hex) {
        var normalized = String(hex || "").replace(/\s+/g, "")
        if (normalized.length < 2) {
            return 0
        }

        var value = parseInt(normalized.substring(0, 2), 16)
        if (!isFinite(value) || value < 0 || value >= protocolOptions.length) {
            return 0
        }
        return value
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

    function uint16At(bytes, offset) {
        if (offset + 1 >= bytes.length) {
            return 0
        }
        return bytes[offset] | (bytes[offset + 1] << 8)
    }

    function wordToHex(value) {
        var normalized = Math.max(motorMin, Math.min(motorMax, Math.round(Number(value))))
        var low = normalized & 0xff
        var high = (normalized >> 8) & 0xff
        return byteToHex(low) + " " + byteToHex(high)
    }

    function byteToHex(value) {
        var hex = (Number(value) & 0xff).toString(16).toUpperCase()
        return hex.length < 2 ? "0" + hex : hex
    }

    function throttlePayloadHex(values) {
        var parts = []
        for (var i = 0; i < 4; ++i) {
            parts.push(wordToHex(values[i]))
        }
        return parts.join(" ")
    }

    function resetMotorValues() {
        motorValues = [motorMin, motorMin, motorMin, motorMin]
    }

    function updateMotorValue(index, value) {
        var next = motorValues.slice()
        next[index] = Math.max(motorMin, Math.min(motorMax, Math.round(Number(value))))
        motorValues = next
        scheduleMotorThrottle()
    }

    function updateAllMotors(value) {
        var throttle = Math.max(motorMin, Math.min(motorMax, Math.round(Number(value))))
        motorValues = [throttle, throttle, throttle, throttle]
        scheduleMotorThrottle()
    }

    function refreshProtocol() {
        if (loadingProtocol || savingProtocol) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone to edit mixer settings"
            return
        }
        if (!flightReady) {
            statusMessage = "Drone must be ready before reading mixer settings"
            return
        }

        loadingProtocol = true
        statusMessage = "Reading ESC protocol"
        droneSession.requestParameterValue(escProtocolParameterId, function(result) {
            var protocol = parseProtocol(result.frame.parameterValueHex)
            loadedProtocol = protocol
            selectedProtocol = protocol
            loadingProtocol = false
            statusMessage = "Mixer settings loaded"
        }, function(error) {
            loadingProtocol = false
            statusMessage = "ESC protocol read failed: " + error.reason
        })
    }

    function cancelChanges() {
        selectedProtocol = loadedProtocol
        statusMessage = droneSession.isOpen ? "Mixer settings restored" : "Connect to a drone to edit mixer settings"
    }

    function saveAndReboot() {
        if (!protocolControlsEnabled) {
            if (droneSession.isOpen && !flightReady) {
                statusMessage = "Drone must be ready before saving mixer settings"
            }
            return
        }

        savingProtocol = true
        statusMessage = "Preparing ESC protocol write"
        store.refreshParameterInfo(escProtocolParameterId, function() {
            if (!store.setParameterValueText(escProtocolParameterId, String(selectedProtocol), "MixerPage")) {
                finishSaveFailure("invalid ESC protocol value")
                return
            }

            statusMessage = "Writing ESC protocol"
            store.writeParameter(escProtocolParameterId, function() {
                statusMessage = "Persisting mixer settings"
                droneSession.saveParameters(function() {
                    statusMessage = "Sending reboot command"
                    droneSession.sendAnotcCommand(connectionSessionBridge.commandRebootCid0,
                                                  connectionSessionBridge.commandRebootCid1,
                                                  connectionSessionBridge.commandRebootCid2,
                                                  "", function() {
                        loadedProtocol = selectedProtocol
                        savingProtocol = false
                        statusMessage = "Mixer settings saved; reboot command sent"
                    }, function(error) {
                        finishSaveFailure("reboot command failed: " + error.reason)
                    })
                }, function(error) {
                    finishSaveFailure("parameter persist failed: " + error.reason)
                })
            }, function(error) {
                finishSaveFailure("parameter write failed: " + error.reason)
            }, "MixerPage")
        }, function(error) {
            finishSaveFailure("parameter info failed: " + error.reason)
        }, "MixerPage")
    }

    function finishSaveFailure(message) {
        savingProtocol = false
        statusMessage = "Mixer settings save failed: " + message
    }

    function setMotorTestMode(enabled) {
        if (!droneSession.isOpen || changingMotorTestMode) {
            return
        }
        if (enabled && !flightReady) {
            statusMessage = "Drone must be ready before enabling motor test mode"
            return
        }

        changingMotorTestMode = true
        statusMessage = enabled ? "Enabling motor test mode" : "Disabling motor test mode"
        droneSession.sendAnotcCommand(connectionSessionBridge.commandToggleMotorTestCid0,
                                      connectionSessionBridge.commandToggleMotorTestCid1,
                                      connectionSessionBridge.commandToggleMotorTestCid2,
                                      byteToHex(enabled ? 1 : 0), function() {
            changingMotorTestMode = false
            motorTestMode = enabled
            if (!enabled) {
                resetMotorValues()
            }
            statusMessage = enabled ? "Motor test mode enabled" : "Motor test mode disabled"
        }, function(error) {
            changingMotorTestMode = false
            statusMessage = "Motor test mode command failed: " + error.reason
        })
    }

    function scheduleMotorThrottle() {
        if (!motorControlsEnabled) {
            return
        }

        throttleRequestPending = true
        if (!throttleSendTimer.running) {
            throttleSendTimer.start()
        }
    }

    function sendMotorThrottle() {
        if (!motorControlsEnabled || !throttleRequestPending) {
            return
        }
        if (throttleRequestActive) {
            return
        }

        throttleRequestPending = false
        throttleRequestActive = true
        statusMessage = "Sending motor throttle"
        droneSession.sendAnotcCommand(connectionSessionBridge.commandMotorTestThrottleCid0,
                                      connectionSessionBridge.commandMotorTestThrottleCid1,
                                      connectionSessionBridge.commandMotorTestThrottleCid2,
                                      throttlePayloadHex(motorValues), function() {
            throttleRequestActive = false
            statusMessage = "Motor throttle sent"
            if (throttleRequestPending) {
                throttleSendTimer.restart()
            }
        }, function(error) {
            throttleRequestActive = false
            statusMessage = "Motor throttle command failed: " + error.reason
            if (throttleRequestPending) {
                throttleSendTimer.restart()
            }
        })
    }

    function handleTelemetryPayload(frame) {
        var bytes = bytesFromHex(frame.payloadHex)
        if (Number(frame.function) === connectionSessionBridge.frameCustomSystemInfo) {
            if (bytes.length > 0) {
                motorTestMode = bytes[0] === connectionSessionBridge.flightStatusMotorTest
                if (!motorTestMode) {
                    resetMotorValues()
                }
            }
        } else if (Number(frame.function) === connectionSessionBridge.framePwm) {
            if (bytes.length >= 8) {
                liveMotorValues = [uint16At(bytes, 0), uint16At(bytes, 2),
                                   uint16At(bytes, 4), uint16At(bytes, 6)]
            }
        } else if (Number(frame.function) === connectionSessionBridge.frameCustomPid) {
            if (bytes.length >= 34) {
                liveMotorValues = [uint16At(bytes, 26), uint16At(bytes, 28),
                                   uint16At(bytes, 30), uint16At(bytes, 32)]
            }
        }
    }

    Component.onCompleted: refreshProtocol()

    Connections {
        target: droneSession

        function onIsOpenChanged() {
            if (droneSession.isOpen) {
                root.refreshProtocol()
            } else {
                root.loadingProtocol = false
                root.savingProtocol = false
                root.changingMotorTestMode = false
                root.throttleRequestActive = false
                root.throttleRequestPending = false
                root.loadedProtocol = 0
                root.selectedProtocol = 0
                root.motorTestMode = false
                root.resetMotorValues()
                root.liveMotorValues = [0, 0, 0, 0]
                root.statusMessage = "Connect to a drone to edit mixer settings"
            }
        }

        function onTelemetryFramesReceived(frames) {
            var payloads = connectionSessionBridge.commandFramePayloads(frames)
            for (var i = 0; i < payloads.length; ++i) {
                root.handleTelemetryPayload(payloads[i])
            }
        }
    }

    Connections {
        target: flight

        function onStatusChanged() {
            if (droneSession.isOpen && root.flightReady) {
                root.refreshProtocol()
            }
        }
    }

    Timer {
        id: throttleSendTimer
        interval: 50
        repeat: false
        onTriggered: root.sendMotorThrottle()
    }

    clip: true
    leftPadding: 5
    rightPadding: 5
    topPadding: 5
    bottomPadding: 5
    contentWidth: availableWidth

    ColumnLayout {
        width: root.availableWidth
        spacing: 8

        GridLayout {
            columns: root.availableWidth >= 980 ? 2 : 1
            columnSpacing: 18
            rowSpacing: 8
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 430
                Layout.alignment: Qt.AlignTop
                spacing: 8

                StyledPanel {
                    title: "Mixer Settings"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 190

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 16

                            Text {
                                text: "Protocol"
                                color: "#2f343b"
                                font.pixelSize: root.pageFontSize
                                Layout.preferredWidth: 130
                            }

                            StyledSelect {
                                id: protocolCombo
                                model: root.protocolOptions
                                enabled: root.protocolControlsEnabled
                                Layout.fillWidth: true

                                Component.onCompleted: currentIndex = root.selectedProtocol

                                onActivated: function(index) {
                                    root.selectedProtocol = index
                                }

                                Connections {
                                    target: root

                                    function onSelectedProtocolChanged() {
                                        if (protocolCombo.currentIndex !== root.selectedProtocol) {
                                            protocolCombo.currentIndex = root.selectedProtocol
                                        }
                                    }
                                }
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
                                enabled: root.protocolControlsEnabled && root.selectedProtocol !== root.loadedProtocol
                                Layout.preferredWidth: 78
                                onClicked: root.cancelChanges()
                            }

                            StyledButton {
                                text: root.savingProtocol ? "Saving..." : "Save & Reboot"
                                styleName: "primary-button"
                                enabled: root.protocolControlsEnabled
                                Layout.preferredWidth: 112
                                onClicked: root.saveAndReboot()
                            }
                        }
                    }
                }

                StyledPanel {
                    title: "Motor View"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300

                    Item {
                        id: motorView
                        anchors.fill: parent
                        anchors.margins: 16

                        Image {
                            id: mixerIcon
                            source: "qrc:/resources/icons/drone-mixer.svg"
                            fillMode: Image.PreserveAspectFit
                            anchors.centerIn: parent
                            width: Math.min(parent.width * 0.34, 150)
                            height: width
                            opacity: 0.92
                        }

                        MotorReadout {
                            label: "Motor 4"
                            value: root.liveMotorValues[3]
                            anchors.left: parent.left
                            anchors.top: parent.top
                        }

                        MotorReadout {
                            label: "Motor 2"
                            value: root.liveMotorValues[1]
                            anchors.right: parent.right
                            anchors.top: parent.top
                        }

                        MotorReadout {
                            label: "Motor 3"
                            value: root.liveMotorValues[2]
                            anchors.left: parent.left
                            anchors.bottom: parent.bottom
                        }

                        MotorReadout {
                            label: "Motor 1"
                            value: root.liveMotorValues[0]
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }

            StyledPanel {
                title: "Motor Test"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 620
                Layout.preferredHeight: Math.max(498, root.height - 16)
                Layout.alignment: Qt.AlignTop

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        Repeater {
                            model: [
                                { "label": "Motor 1", "index": 0 },
                                { "label": "Motor 2", "index": 1 },
                                { "label": "Motor 3", "index": 2 },
                                { "label": "Motor 4", "index": 3 }
                            ]

                            delegate: MotorColumn {
                                label: modelData.label
                                value: root.motorValues[modelData.index]
                                enabled: root.motorControlsEnabled
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                onMoved: function(throttle) {
                                    root.updateMotorValue(modelData.index, throttle)
                                }
                            }
                        }

                        MotorColumn {
                            label: "All"
                            value: root.motorValues[0]
                            enabled: root.motorControlsEnabled
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            onMoved: function(throttle) {
                                root.updateAllMotors(throttle)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        spacing: 10

                        CheckBox {
                            id: testModeCheck
                            checked: root.motorTestMode
                            enabled: root.motorTestModeToggleEnabled
                            text: "Motor Test Mode"
                            font.pixelSize: root.pageFontSize

                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: testModeCheck.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 4
                                color: testModeCheck.enabled ? "#ffffff" : "#fbfcfd"
                                border.color: testModeCheck.checked ? "#4c75f2" : "#cfd5dd"
                                border.width: 1

                                Canvas {
                                    anchors.centerIn: parent
                                    width: 12
                                    height: 9
                                    visible: testModeCheck.checked

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = "#4c75f2"
                                        ctx.lineWidth = 2
                                        ctx.lineCap = "round"
                                        ctx.lineJoin = "round"
                                        ctx.beginPath()
                                        ctx.moveTo(1, 4)
                                        ctx.lineTo(4.5, 7.5)
                                        ctx.lineTo(11, 1)
                                        ctx.stroke()
                                    }
                                }
                            }

                            contentItem: Text {
                                text: testModeCheck.text
                                font: testModeCheck.font
                                color: testModeCheck.enabled ? "#2d333b" : "#9aa3af"
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: testModeCheck.indicator.width + testModeCheck.spacing
                            }

                            onClicked: {
                                var requestedMode = checked
                                checked = root.motorTestMode
                                root.setMotorTestMode(requestedMode)
                            }

                            Connections {
                                target: root

                                function onMotorTestModeChanged() {
                                    if (testModeCheck.checked !== root.motorTestMode) {
                                        testModeCheck.checked = root.motorTestMode
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    component MotorColumn: ColumnLayout {
        id: motorColumn

        property string label: ""
        property int value: root.motorMin
        signal moved(int throttle)

        spacing: 8

        Text {
            text: motorColumn.label
            color: "#2f343b"
            font.pixelSize: root.pageFontSize
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }

        Item {
            id: throttleControl

            property int previewValue: motorColumn.value
            readonly property real ratio: (previewValue - root.motorMin) / (root.motorMax - root.motorMin)
            readonly property real handleSize: 24

            enabled: motorColumn.enabled
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            Layout.preferredWidth: 38

            onVisibleChanged: previewValue = motorColumn.value

            Connections {
                target: motorColumn

                function onValueChanged() {
                    if (!dragArea.pressed) {
                        throttleControl.previewValue = motorColumn.value
                    }
                }
            }

            function valueFromY(position) {
                var travel = Math.max(1, height - handleSize)
                var clamped = Math.max(0, Math.min(travel, position - handleSize / 2))
                var normalized = 1 - clamped / travel
                return Math.round(root.motorMin + normalized * (root.motorMax - root.motorMin))
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 6
                radius: 3
                color: throttleControl.enabled ? "#e6e9ee" : "#edf0f4"
            }

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: Math.round((1 - throttleControl.ratio) * Math.max(0, throttleControl.height - height))
                width: throttleControl.handleSize
                height: throttleControl.handleSize
                radius: 12
                color: throttleControl.enabled ? "#ffffff" : "#f8fafc"
                border.color: throttleControl.enabled ? "#d4d9e0" : "#e3e7ec"
                border.width: 1
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                enabled: motorColumn.enabled
                hoverEnabled: true
                preventStealing: true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor

                onPressed: function(mouse) {
                    mouse.accepted = true
                    throttleControl.previewValue = throttleControl.valueFromY(mouse.y)
                    motorColumn.moved(throttleControl.previewValue)
                }

                onPositionChanged: function(mouse) {
                    if (pressed) {
                        throttleControl.previewValue = throttleControl.valueFromY(mouse.y)
                        motorColumn.moved(throttleControl.previewValue)
                    }
                }

                onReleased: motorColumn.moved(throttleControl.previewValue)
                onCanceled: motorColumn.moved(throttleControl.previewValue)
            }
        }

        Text {
            text: String(Math.round(throttleControl.previewValue))
            color: "#2d333b"
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: 22
        }
    }

    component MotorReadout: Column {
        property string label: ""
        property int value: 0
        readonly property real fillRatio: Math.max(0, Math.min(1, Number(value) / 1000))

        width: Math.min(116, motorView.width * 0.28)
        spacing: 8

        Rectangle {
            width: parent.width
            height: 36
            radius: 4
            color: "#ffffff"
            border.color: "#d7dce3"
            border.width: 1
            clip: true

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: Math.round(parent.width * fillRatio)
                color: "#dbe7ff"
                visible: fillRatio > 0
            }

            Text {
                anchors.centerIn: parent
                text: String(value)
                color: "#2d333b"
                font.pixelSize: 15
            }
        }

        Text {
            width: parent.width
            text: label
            color: "#69717d"
            font.pixelSize: root.pageFontSize
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
