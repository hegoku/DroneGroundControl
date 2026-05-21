import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

ScrollView {
    id: root

    readonly property int pageFontSize: 13
    readonly property int tableFontSize: 13
    readonly property int tableHeaderFontSize: 13
    readonly property int tableRowHeight: 28

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: root.availableWidth
        spacing: 14

        StyledPanel {
            title: "Settings"
            Layout.fillWidth: true
            Layout.preferredHeight: 140

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20

                    Text {
                        text: "Rotation"
                        color: "#2f343b"
                        font.pixelSize: root.pageFontSize
                        Layout.preferredWidth: 210
                    }

                    StyledSelect {
                        model: ["0°", "90°", "180°", "270°"]
                        currentIndex: 0
                        enabled: droneSession.isOpen
                        Layout.fillWidth: true
                        Layout.maximumWidth: 300
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
                        enabled: droneSession.isOpen
                        Layout.preferredWidth: 88
                    }

                    StyledButton {
                        text: "Save"
                        styleName: "primary-button"
                        enabled: droneSession.isOpen
                        Layout.preferredWidth: 88
                    }
                }
            }
        }

        StyledPanel {
            title: "Magnetometer Calibration"
            Layout.fillWidth: true
            Layout.preferredHeight: 260

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 14

                CalibrationTable {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                }

                Item { Layout.fillHeight: true }

                StyledProgressBar {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.preferredHeight: 7
                    value: 0.45
                    progressColor: "#2458f2"
                    enabled: droneSession.isOpen
                }

                StyledButton {
                    text: "Calibrate Magnetometer"
                    styleName: "primary-button"
                    enabled: droneSession.isOpen
                    Layout.fillWidth: true
                    Layout.maximumWidth: 760
                    Layout.preferredHeight: 24
                }
            }
        }
    }

    component CalibrationTable: Rectangle {
        implicitHeight: root.tableRowHeight * 4
        radius: 4
        color: "#ffffff"
        border.color: "#d8dde4"
        border.width: 1
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            TableRow { axis: ""; scale: "Scale"; offset: "Offset"; header: true; Layout.fillWidth: true; Layout.preferredHeight: root.tableRowHeight }
            TableRow { axis: "X"; scale: "0.3423432"; offset: "0.42352"; Layout.fillWidth: true; Layout.preferredHeight: root.tableRowHeight }
            TableRow { axis: "Y"; scale: "3.432432"; offset: "4.532"; Layout.fillWidth: true; Layout.preferredHeight: root.tableRowHeight }
            TableRow { axis: "Z"; scale: "0.3423"; offset: "42.432423"; Layout.fillWidth: true; Layout.preferredHeight: root.tableRowHeight }
        }
    }

    component TableRow: Item {
        id: tableRow

        property string axis: ""
        property string scale: ""
        property string offset: ""
        property bool header: false

        RowLayout {
            anchors.fill: parent
            spacing: 0

            TableCell { text: tableRow.axis; header: tableRow.header; Layout.preferredWidth: 90; Layout.fillHeight: true }
            TableCell { text: tableRow.scale; header: tableRow.header; Layout.fillWidth: true; Layout.fillHeight: true }
            TableCell { text: tableRow.offset; header: tableRow.header; showDivider: false; Layout.fillWidth: true; Layout.fillHeight: true }
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
