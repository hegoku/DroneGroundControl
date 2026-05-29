pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import DroneGroundControl
import "../components"

Window {
    id: root

    width: 1120
    height: 620
    minimumWidth: 900
    minimumHeight: 560
    visible: false
    title: "RC Simulator"
    color: "#fbfcfd"

    readonly property int rcMin: 1000
    readonly property int rcMax: 2000
    readonly property var controllerOptions: ["Mouse", "Gamepad"]
    readonly property var gamepadOptions: ["No gamepad connected"]
    readonly property var modeOptions: ["Ready", "Angle", "Rate"]
    readonly property int controllerGamepadIndex: 1

    property bool controlOpen: true
    property int controllerIndex: 1
    property int gamepadIndex: 0
    readonly property int modeIndex: modeIndexForStatus(flight.status)
    property int yaw: 1500
    property int roll: 1500
    property int throttle: 1000
    property int pitch: 1500
    readonly property bool flightReady: flight.status === Flight.FLIGHT_STATUS_READY
    readonly property bool flightAngleMode: flight.status === Flight.FLIGHT_STATUS_ANGLE_MODE
    readonly property bool flightRateMode: flight.status === Flight.FLIGHT_STATUS_ANGLE_RATE_MODE
    readonly property bool flightRcMode: flightReady || flightAngleMode || flightRateMode
    readonly property bool gamepadConnected: gamepadOptions.length > 0 && gamepadOptions[gamepadIndex] !== "No gamepad connected"
    readonly property bool selectedControllerAvailable: controllerIndex !== controllerGamepadIndex || gamepadConnected
    readonly property bool controllerControlsEnabled: !controlOpen
    readonly property bool gamepadControlsEnabled: controllerControlsEnabled && controllerIndex === controllerGamepadIndex
    readonly property bool rcControlsEnabled: controlOpen && droneSession.isOpen && flightRcMode && selectedControllerAvailable

    function clampRc(value) {
        return Math.max(rcMin, Math.min(rcMax, Math.round(Number(value))))
    }

    function modeIndexForStatus(status) {
        if (status === Flight.FLIGHT_STATUS_ANGLE_MODE) {
            return 1
        }
        if (status === Flight.FLIGHT_STATUS_ANGLE_RATE_MODE) {
            return 2
        }
        return 0
    }

    onVisibleChanged: {
        if (visible) {
            controlOpen = droneSession.isOpen && flightReady && selectedControllerAvailable
            raise()
            requestActivate()
        }
    }

    Connections {
        target: droneSession

        function onIsOpenChanged() {
            if (!droneSession.isOpen) {
                root.controlOpen = false
            }
        }
    }

    Connections {
        target: flight

        function onStatusChanged() {
            if (root.controlOpen && (!root.flightRcMode || !root.selectedControllerAvailable)) {
                root.controlOpen = false
            }
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 18

        Rectangle {
            id: openPanel
            width: 162
            height: 98
            radius: 8
            color: "#ffffff"
            border.color: "#e1e5eb"
            border.width: 1

            Row {
                anchors.centerIn: parent
                spacing: 14

                ToggleSwitch {
                    checked: root.controlOpen
                    enabled: droneSession.isOpen && root.selectedControllerAvailable && (root.controlOpen || root.flightReady)
                    anchors.verticalCenter: parent.verticalCenter
                    onToggled: function(checked) {
                        if (checked && (!root.flightReady || !root.selectedControllerAvailable)) {
                            return
                        }
                        root.controlOpen = checked
                    }
                }

                Text {
                    text: "Open"
                    color: "#2f343b"
                    font.pixelSize: 16
                    verticalAlignment: Text.AlignVCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        Rectangle {
            id: optionsPanel
            anchors.left: openPanel.right
            anchors.leftMargin: 20
            anchors.right: parent.right
            height: openPanel.height
            radius: 8
            color: "#ffffff"
            border.color: "#e1e5eb"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 14
                anchors.bottomMargin: 14
                spacing: 24

                ControlGroup {
                    label: "Controller"
                    Layout.preferredWidth: 250
                    Layout.fillHeight: true

                    StyledSelect {
                        model: root.controllerOptions
                        currentIndex: root.controllerIndex
                        enabled: root.controllerControlsEnabled
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        onActivated: function(index) {
                            root.controllerIndex = index
                        }
                    }
                }

                ControlGroup {
                    label: "Gamepad"
                    Layout.preferredWidth: 270
                    Layout.fillHeight: true

                    StyledSelect {
                        model: root.gamepadOptions
                        currentIndex: root.gamepadIndex
                        enabled: root.gamepadControlsEnabled
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        onActivated: function(index) {
                            root.gamepadIndex = index
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#e1e5eb"
                }

                ControlGroup {
                    label: "Mode"
                    Layout.preferredWidth: 290
                    Layout.fillHeight: true

                    ModeSegment {
                        options: root.modeOptions
                        currentIndex: root.modeIndex
                        enabled: root.rcControlsEnabled
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }

        Rectangle {
            id: controlPanel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: openPanel.bottom
            anchors.topMargin: 20
            anchors.bottom: parent.bottom
            radius: 8
            color: "#ffffff"
            border.color: "#e1e5eb"
            border.width: 1

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 32
                width: 1
                height: parent.height - 64
                color: "#e1e5eb"
            }

            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 62
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: 44
                y: 174
                height: 1
                color: "#e1e5eb"
            }

            HorizontalRcControl {
                label: "Yaw"
                value: root.yaw
                enabled: root.rcControlsEnabled
                anchors.left: parent.left
                anchors.leftMargin: 78
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: 62
                anchors.top: parent.top
                anchors.topMargin: 26
                height: 104
                onValueEdited: function(value) {
                    root.yaw = value
                }
            }

            VerticalRcControl {
                label: "Throttle"
                value: root.throttle
                enabled: root.rcControlsEnabled
                anchors.left: parent.left
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 200
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 28
                onValueEdited: function(value) {
                    root.throttle = value
                }
            }

            ControlPad {
                xLabel: "Roll"
                yLabel: "Pitch"
                minimumValue: root.rcMin
                maximumValue: root.rcMax
                xValue: root.roll
                yValue: root.pitch
                defaultXValue: 1500
                defaultYValue: 1500
                returnXToDefaultOnRelease: true
                returnYToDefaultOnRelease: true
                enabled: root.rcControlsEnabled
                anchors.left: parent.horizontalCenter
                anchors.leftMargin: 46
                anchors.right: parent.right
                anchors.rightMargin: 54
                anchors.top: parent.top
                anchors.topMargin: 38
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 38
                onValuesEdited: function(xValue, yValue) {
                    root.roll = xValue
                    root.pitch = yValue
                }
            }
        }
    }

    component ControlGroup: ColumnLayout {
        id: controlGroup

        property string label: ""

        spacing: 8

        Text {
            text: controlGroup.label
            color: "#4b5563"
            font.pixelSize: 14
            Layout.fillWidth: true
        }
    }

    component ToggleSwitch: Item {
        id: toggle

        property bool checked: false
        signal toggled(bool checked)

        implicitWidth: 52
        implicitHeight: 28
        activeFocusOnTab: true
        opacity: enabled ? 1.0 : 0.55

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: toggle.checked ? "#4c75f2" : "#e5e7eb"
            border.color: toggle.checked ? "#3f67dc" : "#cfd5dd"
            border.width: 1
        }

        Rectangle {
            width: 24
            height: 24
            radius: 12
            x: toggle.checked ? toggle.width - width - 2 : 2
            y: 2
            color: "#ffffff"
            border.color: "#d8dde5"
            border.width: 1
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: toggle.toggled(!toggle.checked)
        }

        Keys.onReturnPressed: toggle.toggled(!toggle.checked)
        Keys.onSpacePressed: toggle.toggled(!toggle.checked)
    }

    component ModeSegment: Item {
        id: segment

        property var options: []
        property int currentIndex: 0
        signal activated(int index)

        implicitHeight: 34
        opacity: enabled ? 1.0 : 0.55

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "#ffffff"
            border.color: "#cfd5dd"
            border.width: 1
            clip: true

            Row {
                anchors.fill: parent

                Repeater {
                    model: segment.options

                    delegate: Rectangle {
                        id: segmentButton

                        required property int index
                        required property string modelData

                        width: segment.width / Math.max(1, segment.options.length)
                        height: segment.height
                        color: index === segment.currentIndex ? "#4c75f2" : "#ffffff"
                        border.color: index === segment.currentIndex ? "#3f67dc" : "#e1e5eb"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: segmentButton.modelData
                            color: segmentButton.index === segment.currentIndex ? "#ffffff" : "#4b5563"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: segment.enabled
                            hoverEnabled: true
                            cursorShape: segment.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: segment.activated(segmentButton.index)
                        }
                    }
                }
            }
        }
    }

    component HorizontalRcControl: Item {
        id: rcControl

        property string label: ""
        property int value: 1500
        signal valueEdited(int value)

        opacity: enabled ? 1.0 : 0.55

        Text {
            text: rcControl.value
            color: "#18325f"
            font.pixelSize: 18
            anchors.horizontalCenter: slider.horizontalCenter
            anchors.top: parent.top
        }

        Text {
            text: rcControl.label
            color: "#2f343b"
            font.pixelSize: 15
            anchors.left: parent.left
            anchors.verticalCenter: slider.verticalCenter
        }

        Slider {
            id: slider
            from: root.rcMin
            to: root.rcMax
            stepSize: 1
            value: rcControl.value
            enabled: rcControl.enabled
            anchors.left: parent.left
            anchors.leftMargin: 52
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: 34
            onMoved: rcControl.valueEdited(Math.round(value))

            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 6
                radius: 3
                color: "#e3e7ed"

                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: "#4c75f2"
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 24
                height: 24
                radius: 12
                color: "#ffffff"
                border.color: "#d7dce4"
                border.width: 1
            }
        }

        Text {
            text: root.rcMin
            color: "#6b7280"
            font.pixelSize: 13
            anchors.left: slider.left
            anchors.bottom: parent.bottom
        }

        Text {
            text: root.rcMax
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: slider.right
            anchors.bottom: parent.bottom
        }
    }

    component VerticalRcControl: Item {
        id: rcControl

        property string label: ""
        property int value: 1000
        signal valueEdited(int value)

        opacity: enabled ? 1.0 : 0.55

        Slider {
            id: verticalSlider
            orientation: Qt.Vertical
            from: root.rcMin
            to: root.rcMax
            stepSize: 1
            value: rcControl.value
            enabled: rcControl.enabled
            width: 34
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: labelText.top
            anchors.bottomMargin: 18
            onMoved: rcControl.valueEdited(Math.round(value))

            background: Rectangle {
                x: verticalSlider.leftPadding + verticalSlider.availableWidth / 2 - width / 2
                y: verticalSlider.topPadding
                width: 6
                height: verticalSlider.availableHeight
                radius: 3
                color: "#e3e7ed"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: (1 - verticalSlider.visualPosition) * parent.height
                    radius: parent.radius
                    color: "#4c75f2"
                }
            }

            handle: Rectangle {
                id: verticalHandle

                x: verticalSlider.leftPadding + verticalSlider.availableWidth / 2 - width / 2
                y: verticalSlider.topPadding + verticalSlider.visualPosition * (verticalSlider.availableHeight - height)
                width: 24
                height: 24
                radius: 12
                color: "#ffffff"
                border.color: "#d7dce4"
                border.width: 1
            }
        }

        Text {
            text: root.rcMax
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: verticalSlider.left
            anchors.rightMargin: 14
            anchors.top: verticalSlider.top
        }

        Text {
            text: root.rcMin
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: verticalSlider.left
            anchors.rightMargin: 14
            anchors.bottom: verticalSlider.bottom
        }

        Text {
            text: rcControl.value
            color: "#18325f"
            font.pixelSize: 18
            anchors.left: verticalSlider.right
            anchors.leftMargin: 18
            anchors.verticalCenter: verticalHandle.verticalCenter
        }

        Text {
            id: labelText
            text: rcControl.label
            color: "#2f343b"
            font.pixelSize: 15
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
        }
    }
}
