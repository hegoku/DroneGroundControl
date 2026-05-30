import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Rectangle {
    id: root

    anchors.fill: parent
    color: "#fbfcfd"

    function formatAngle(value) {
        var number = Number(value)
        if (!isFinite(number)) {
            number = 0
        }
        return number.toFixed(2) + "\u00b0"
    }

    function formatHeading(value) {
        var number = Number(value) % 360
        if (number < 0) {
            number += 360
        }
        return number.toFixed(2) + "\u00b0"
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 5
        clip: true

        ColumnLayout {
            width: Math.max(root.width - 36, 640)
            spacing: 18

            StyledPanel {
                title: "Ground Control"
                Layout.fillWidth: true
                Layout.preferredHeight: 490

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 24
                    spacing: 32

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 330

                        AttitudeIndicator {
                            anchors.centerIn: parent
                            width: Math.min(parent.width, parent.height)
                            height: width
                            roll: flight.roll
                            pitch: flight.pitch
                            heading: flight.yaw
                        }
                    }

                    ColumnLayout {
                        Layout.preferredWidth: 240
                        Layout.maximumWidth: 280
                        Layout.fillHeight: true
                        spacing: 10

                        Text {
                            text: "Live Instruments"
                            color: "#23272f"
                            font.pixelSize: 20
                            font.bold: true
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Attitude and heading from the connected flight controller."
                            color: "#6b7280"
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }

                        Compass {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 158
                            Layout.preferredHeight: 158
                            heading: flight.yaw
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 34
                            radius: 17
                            color: droneSession.isOpen ? "#e9f8ed" : "#f3f4f6"
                            border.color: droneSession.isOpen ? "#b7e3c0" : "#d8dde5"
                            border.width: 1

                            Row {
                                anchors.centerIn: parent
                                spacing: 8

                                Rectangle {
                                    width: 9
                                    height: 9
                                    radius: width / 2
                                    color: droneSession.isOpen ? "#32a852" : "#9ca3af"
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    text: droneSession.isOpen ? "Live telemetry" : "Waiting for connection"
                                    color: droneSession.isOpen ? "#246b34" : "#6b7280"
                                    font.pixelSize: 12
                                    font.bold: true
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: "#e4e7eb"
                        }

                        TelemetryValue {
                            label: "Roll"
                            value: root.formatAngle(flight.roll)
                        }

                        TelemetryValue {
                            label: "Pitch"
                            value: root.formatAngle(flight.pitch)
                        }

                        TelemetryValue {
                            label: "Heading"
                            value: root.formatHeading(flight.yaw)
                        }

                        Item {
                            Layout.fillHeight: true
                        }
                    }
                }
            }

            StyledPanel {
                title: "Instrument Guide"
                Layout.fillWidth: true
                Layout.preferredHeight: 104

                Text {
                    anchors.fill: parent
                    anchors.margins: 18
                    text: "The horizon responds to pitch and roll while the aircraft marker stays fixed. The compass shows normalized yaw from 0\u00b0-359\u00b0."
                    color: "#5f6878"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    component TelemetryValue: RowLayout {
        id: telemetryValue

        required property string label
        required property string value

        Layout.fillWidth: true
        spacing: 12

        Text {
            text: telemetryValue.label
            color: "#6b7280"
            font.pixelSize: 13
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            text: telemetryValue.value
            color: "#18325f"
            font.pixelSize: 17
            font.bold: true
        }
    }
}
