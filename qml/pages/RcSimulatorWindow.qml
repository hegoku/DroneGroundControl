pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
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
    readonly property int rcSendIntervalMs: 10
    readonly property int pcRcCommandCid0: 0
    readonly property int pcRcCommandCid1: 0
    readonly property int pcRcCommandCid2: 6
    readonly property var controllerOptions: ["Mouse", "Gamepad"]
    readonly property var gamepadOptions: ["No gamepad connected"]
    readonly property var modeOptions: ["Ready", "Angle", "Rate"]
    readonly property int controllerGamepadIndex: 1

    property bool controlOpen: false
    property int controllerIndex: 0
    property int gamepadIndex: 0
    property int modeIndex: 0
    property int yaw: 1500
    property int roll: 1500
    property int throttle: 1000
    property int pitch: 1500
    property int aux1: 1000
    readonly property bool gamepadConnected: gamepadOptions.length > 0 && gamepadOptions[gamepadIndex] !== "No gamepad connected"
    readonly property bool selectedControllerAvailable: controllerIndex !== controllerGamepadIndex || gamepadConnected
    readonly property bool controllerControlsEnabled: !controlOpen
    readonly property bool gamepadControlsEnabled: controllerControlsEnabled && controllerIndex === controllerGamepadIndex
    readonly property bool rcControlsEnabled: controlOpen && droneSession.isOpen && selectedControllerAvailable
    readonly property bool rcStreamingEnabled: visible && rcControlsEnabled

    function clampRc(value) {
        return Math.max(rcMin, Math.min(rcMax, Math.round(Number(value))))
    }

    function byteToHex(value) {
        var hex = (Number(value) & 0xff).toString(16).toUpperCase()
        return hex.length < 2 ? "0" + hex : hex
    }

    function wordToHex(value) {
        var channel = clampRc(value)
        return byteToHex(channel & 0xff) + " " + byteToHex((channel >> 8) & 0xff)
    }

    function modeAux2Value() {
        if (modeIndex === 1) {
            return 1500
        }
        if (modeIndex === 2) {
            return 2000
        }
        return 1000
    }

    function pcRcPayloadHex() {
        return [
            wordToHex(roll),
            wordToHex(pitch),
            wordToHex(throttle),
            wordToHex(yaw),
            wordToHex(aux1),
            wordToHex(modeAux2Value())
        ].join(" ")
    }

    function sendRcChannels() {
        if (!rcStreamingEnabled) {
            return
        }

        droneSession.sendAnotcCommand(pcRcCommandCid0,
                                      pcRcCommandCid1,
                                      pcRcCommandCid2,
                                      pcRcPayloadHex())
    }

    onVisibleChanged: {
        if (visible) {
            raise()
            requestActivate()
        } else {
            controlOpen = false
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

    onSelectedControllerAvailableChanged: {
        if (!selectedControllerAvailable) {
            controlOpen = false
        }
    }

    Timer {
        interval: root.rcSendIntervalMs
        repeat: true
        running: root.rcStreamingEnabled
        triggeredOnStart: true
        onTriggered: root.sendRcChannels()
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
                    enabled: droneSession.isOpen && root.selectedControllerAvailable
                    anchors.verticalCenter: parent.verticalCenter
                    onToggled: function(checked) {
                        if (checked && (!droneSession.isOpen || !root.selectedControllerAvailable)) {
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
                        onActivated: function(index) {
                            root.modeIndex = index
                        }
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
        readonly property real valueRatio: (root.clampRc(value) - root.rcMin) / (root.rcMax - root.rcMin)
        signal valueEdited(int value)

        opacity: enabled ? 1.0 : 0.55

        function updateFromTrackX(localX) {
            var ratio = Math.max(0, Math.min(1, localX / track.width))
            valueEdited(root.clampRc(root.rcMin + ratio * (root.rcMax - root.rcMin)))
        }

        Text {
            text: rcControl.value
            color: "#18325f"
            font.pixelSize: 18
            anchors.horizontalCenter: track.horizontalCenter
            anchors.top: parent.top
        }

        Text {
            text: rcControl.label
            color: "#2f343b"
            font.pixelSize: 15
            anchors.left: parent.left
            anchors.verticalCenter: track.verticalCenter
        }

        Item {
            id: track
            anchors.left: parent.left
            anchors.leftMargin: 52
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: 34

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 6
                radius: 3
                color: "#e3e7ed"

                Rectangle {
                    width: rcControl.valueRatio * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: "#4c75f2"
                }
            }

            Rectangle {
                x: rcControl.valueRatio * (track.width - width)
                anchors.verticalCenter: parent.verticalCenter
                width: 24
                height: 24
                radius: 12
                color: "#ffffff"
                border.color: "#d7dce4"
                border.width: 1
            }

            MouseArea {
                anchors.fill: parent
                enabled: rcControl.enabled
                hoverEnabled: true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onPressed: function(mouse) {
                    rcControl.updateFromTrackX(mouse.x)
                }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        rcControl.updateFromTrackX(mouse.x)
                    }
                }
            }
        }

        Text {
            text: root.rcMin
            color: "#6b7280"
            font.pixelSize: 13
            anchors.left: track.left
            anchors.bottom: parent.bottom
        }

        Text {
            text: root.rcMax
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: track.right
            anchors.bottom: parent.bottom
        }
    }

    component VerticalRcControl: Item {
        id: rcControl

        property string label: ""
        property int value: 1000
        readonly property real valueRatio: (root.clampRc(value) - root.rcMin) / (root.rcMax - root.rcMin)
        signal valueEdited(int value)

        opacity: enabled ? 1.0 : 0.55

        function updateFromTrackY(localY) {
            var ratio = Math.max(0, Math.min(1, localY / verticalTrack.height))
            valueEdited(root.clampRc(root.rcMax - ratio * (root.rcMax - root.rcMin)))
        }

        Item {
            id: verticalTrack
            width: 34
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.bottom: labelText.top
            anchors.bottomMargin: 18

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 6
                height: parent.height
                radius: 3
                color: "#e3e7ed"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: rcControl.valueRatio * parent.height
                    radius: parent.radius
                    color: "#4c75f2"
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled: rcControl.enabled
                hoverEnabled: true
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onPressed: function(mouse) {
                    rcControl.updateFromTrackY(mouse.y)
                }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        rcControl.updateFromTrackY(mouse.y)
                    }
                }
            }
        }

        Rectangle {
            id: verticalHandle

            x: verticalTrack.x + verticalTrack.width / 2 - width / 2
            y: verticalTrack.y + (1 - rcControl.valueRatio) * (verticalTrack.height - height)
            width: 24
            height: 24
            radius: 12
            color: "#ffffff"
            border.color: "#d7dce4"
            border.width: 1
        }

        Text {
            text: root.rcMax
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: verticalTrack.left
            anchors.rightMargin: 14
            anchors.top: verticalTrack.top
        }

        Text {
            text: root.rcMin
            color: "#6b7280"
            font.pixelSize: 13
            anchors.right: verticalTrack.left
            anchors.rightMargin: 14
            anchors.bottom: verticalTrack.bottom
        }

        Text {
            text: rcControl.value
            color: "#18325f"
            font.pixelSize: 18
            anchors.left: verticalTrack.right
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
