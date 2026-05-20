import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#fbfcfd"

    readonly property int rcProtocolParameterId: 59
    readonly property var protocolOptions: ["IBUS", "PC"]
    property var store: droneSession.parameterStore
    property int loadedProtocol: 0
    property int selectedProtocol: 0
    property bool loadingProtocol: false
    property bool savingProtocol: false
    property string statusMessage: droneSession.isOpen ? "Ready" : "Connect to a drone to edit RC settings"

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

    function refreshProtocol() {
        if (loadingProtocol || savingProtocol) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone to edit RC settings"
            return
        }

        loadingProtocol = true
        statusMessage = "Reading RC protocol"
        droneSession.requestParameterValue(rcProtocolParameterId, function(result) {
            var protocol = parseProtocol(result.frame.parameterValueHex)
            loadedProtocol = protocol
            selectedProtocol = protocol
            loadingProtocol = false
            statusMessage = "RC protocol loaded"
        }, function(error) {
            loadingProtocol = false
            statusMessage = "RC protocol read failed: " + error.reason
        })
    }

    function cancelChanges() {
        selectedProtocol = loadedProtocol
        statusMessage = droneSession.isOpen ? "RC protocol restored" : "Connect to a drone to edit RC settings"
    }

    function saveAndReboot() {
        if (loadingProtocol || savingProtocol) {
            return
        }
        if (!droneSession.isOpen) {
            statusMessage = "Connect to a drone before saving RC settings"
            return
        }

        savingProtocol = true
        saveRcProtocolAndReboot()
    }

    function finishSaveFailure(message) {
        savingProtocol = false
        statusMessage = "RC protocol save failed: " + message
    }

    function saveRcProtocolAndReboot() {
        statusMessage = "Preparing RC protocol write"
        store.refreshParameterInfo(rcProtocolParameterId, function() {
            if (!store.setParameterValueText(rcProtocolParameterId, String(selectedProtocol), "RCPage")) {
                finishSaveFailure("invalid RC protocol value")
                return
            }

            statusMessage = "Writing RC protocol"
            store.writeParameter(rcProtocolParameterId, function() {
                statusMessage = "Persisting RC settings"
                droneSession.saveParameters(function() {
                    statusMessage = "Sending reboot command"
                    droneSession.sendAnotcCommand(0, 0, 3, "", function() {
                        loadedProtocol = selectedProtocol
                        savingProtocol = false
                        statusMessage = "RC protocol saved; reboot command sent"
                    }, function(error) {
                        finishSaveFailure("reboot command failed: " + error.reason)
                    })
                }, function(error) {
                    finishSaveFailure("parameter persist failed: " + error.reason)
                })
            }, function(error) {
                finishSaveFailure("parameter write failed: " + error.reason)
            }, "RCPage")
        }, function(error) {
            savingProtocol = false
            statusMessage = "RC protocol save failed: parameter info failed: " + error.reason
        }, "RCPage")
    }

    Component.onCompleted: refreshProtocol()

    Connections {
        target: droneSession

        function onIsOpenChanged() {
            if (droneSession.isOpen) {
                refreshProtocol()
            } else {
                loadingProtocol = false
                savingProtocol = false
                loadedProtocol = 0
                selectedProtocol = 0
                statusMessage = "Connect to a drone to edit RC settings"
            }
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 18
        clip: true

        ColumnLayout {
            width: Math.max(root.width - 36, 640)
            spacing: 0

            GridLayout {
                columns: root.width >= 980 ? 2 : 1
                columnSpacing: 24
                rowSpacing: 18
                Layout.fillWidth: true

                StyledPanel {
                    title: "RC Settings"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 420
                    Layout.preferredHeight: 220
                    Layout.minimumWidth: 320
                    Layout.alignment: Qt.AlignTop

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 18

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 18

                            Text {
                                text: "Protocol"
                                color: "#2f343b"
                                font.pixelSize: 16
                                Layout.preferredWidth: 130
                            }

                            StyledSelect {
                                id: protocolCombo
                                model: root.protocolOptions
                                currentIndex: root.selectedProtocol
                                enabled: droneSession.isOpen && !root.loadingProtocol && !root.savingProtocol
                                Layout.fillWidth: true
                                Layout.preferredHeight: 36

                                onActivated: function(index) {
                                    root.selectedProtocol = index
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#e1e5eb"
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Item { Layout.fillWidth: true }

                            StyledButton {
                                text: "Cancel"
                                styleName: "cancel-button"
                                enabled: droneSession.isOpen && !root.loadingProtocol && !root.savingProtocol
                                         && root.selectedProtocol !== root.loadedProtocol
                                Layout.preferredWidth: 86
                                Layout.preferredHeight: 36
                                onClicked: root.cancelChanges()
                            }

                            StyledButton {
                                text: root.savingProtocol ? "Saving..." : "Save & Reboot"
                                styleName: "primary-button"
                                enabled: droneSession.isOpen && !root.loadingProtocol && !root.savingProtocol
                                Layout.preferredWidth: 128
                                Layout.preferredHeight: 36
                                onClicked: root.saveAndReboot()
                            }
                        }
                    }
                }

                StyledPanel {
                    title: "RC Channels"
                    Layout.fillWidth: true
                    Layout.preferredWidth: 560
                    Layout.preferredHeight: Math.max(360, root.height - 36)
                    Layout.minimumWidth: 360
                    Layout.alignment: Qt.AlignTop

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10

                        Repeater {
                            model: rcChannelModel

                            delegate: RowLayout {
                                id: channelRow

                                required property string label
                                required property string valueText
                                required property real normalized
                                required property bool active

                                Layout.fillWidth: true
                                height: 34
                                spacing: 16

                                Text {
                                    text: label
                                    color: "#333942"
                                    font.pixelSize: 15
                                    Layout.preferredWidth: 90
                                }

                                RcChannelBar {
                                    valueText: channelRow.valueText
                                    normalized: channelRow.normalized
                                    active: channelRow.active
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    component RcChannelBar: Rectangle {
        property string valueText: "0"
        property real normalized: 0
        property bool active: false

        radius: 4
        color: "#ffffff"
        border.color: "#d9dee5"
        border.width: 1
        clip: true

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 2
            width: Math.max(0, (parent.width - 4) * parent.normalized)
            radius: 3
            color: parent.active ? "#f6b13d" : "transparent"
        }

        Text {
            anchors.centerIn: parent
            text: parent.valueText
            color: parent.active ? "#1f2933" : "#6b7280"
            font.pixelSize: 15
        }
    }

}
