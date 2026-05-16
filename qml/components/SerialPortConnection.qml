import QtQuick 2.15
import QtQuick.Controls.Basic

Item {
    id: serialConnection

    property var availablePorts: []
    property string portName: availablePorts.length > 0 ? portCombo.currentText : ""
    property string baudRate: baudRateCombo.currentText
    property string dataBits: dataBitsCombo.currentText
    property string parity: parityCombo.currentText
    property string stopBit: stopBitCombo.currentText

    signal refreshRequested()

    width: 254
    height: 34

    Row {
        id: controlRow
        anchors.fill: parent
        spacing: 8

        ComboBox {
            id: portCombo
            width: 170
            height: parent.height
            enabled: serialConnection.availablePorts.length > 0
            model: serialConnection.availablePorts.length > 0
                   ? serialConnection.availablePorts
                   : [qsTr("No serial ports")]
            font.pixelSize: 14
            leftPadding: 8
            rightPadding: 20

            contentItem: Item {
                clip: true

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: portCombo.leftPadding
                    anchors.rightMargin: portCombo.rightPadding
                    text: portCombo.currentText
                    color: "white"
                    font: portCombo.font
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            indicator: Canvas {
                x: portCombo.width - width - 7
                y: (portCombo.height - height) / 2
                width: 10
                height: 7

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "white"
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(1, 1)
                    ctx.lineTo(width / 2, height - 1)
                    ctx.lineTo(width - 1, 1)
                    ctx.stroke()
                }
            }

            background: Rectangle {
                radius: 6
                color: "#1e1e1e"
                border.color: portCombo.hovered || portCombo.activeFocus ? "#4c8bf5" : "#424242"
                border.width: 1
            }

            popup: Popup {
                y: portCombo.height + 4
                width: portCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 0

                contentItem: ListView {
                    clip: true
                    implicitHeight: Math.min(contentHeight, 160)
                    model: portCombo.popup.visible ? portCombo.delegateModel : null
                    currentIndex: portCombo.highlightedIndex
                }

                background: Rectangle {
                    color: "#202020"
                    radius: 6
                    border.color: "#3f3f3f"
                }
            }

            delegate: ItemDelegate {
                width: portCombo.width
                height: 30
                contentItem: Text {
                    text: modelData
                    color: "white"
                    font.pixelSize: 13
                    leftPadding: 8
                    rightPadding: 8
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: highlighted ? "#4c8bf5" : "transparent"
                }
            }
        }

        Rectangle {
            id: refreshButton
            width: 34
            height: parent.height
            radius: 6
            color: refreshArea.pressed ? "#303030" : "#1e1e1e"
            border.color: refreshArea.containsMouse ? "#4c8bf5" : "#424242"
            border.width: 1

            Image {
                anchors.centerIn: parent
                width: 16
                height: 16
                source: "qrc:/resources/icons/refresh.svg"
                sourceSize.width: 16
                sourceSize.height: 16
                fillMode: Image.PreserveAspectFit
            }

            MouseArea {
                id: refreshArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: serialConnection.refreshRequested()
            }
        }

        Rectangle {
            id: settingsButton
            width: 34
            height: parent.height
            radius: 6
            color: settingsArea.pressed || settingsPopup.opened ? "#303030" : "#1e1e1e"
            border.color: settingsArea.containsMouse || settingsPopup.opened ? "#4c8bf5" : "#424242"
            border.width: 1

            Canvas {
                anchors.centerIn: parent
                width: 11
                height: 8

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "white"
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(1, 1)
                    ctx.lineTo(width / 2, height - 1)
                    ctx.lineTo(width - 1, 1)
                    ctx.stroke()
                }
            }

            MouseArea {
                id: settingsArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: settingsPopup.opened ? settingsPopup.close() : settingsPopup.open()
            }
        }
    }

    Popup {
        id: settingsPopup
        x: settingsButton.x + settingsButton.width / 2 - width / 2
        y: settingsButton.y + settingsButton.height + 8
        width: 250
        height: 185
        padding: 0
        modal: false
        focus: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Item {}

        contentItem: Item {
            Rectangle {
                id: shadow
                anchors.fill: panel
                anchors.topMargin: 3
                anchors.leftMargin: 0
                radius: panel.radius
                color: "#000000"
                opacity: 0.35
            }

            Rectangle {
                id: arrow
                width: 14
                height: 14
                x: settingsPopup.width / 2 - width / 2
                y: 0
                rotation: 45
                color: "#202020"
                border.color: "#353535"
                border.width: 1
            }

            Rectangle {
                id: panel
                x: 0
                y: 7
                width: parent.width
                height: parent.height - 7
                radius: 8
                color: "#202020"
                border.color: "#353535"
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 9

                    SettingsRow {
                        label: "BaudRate"
                        comboModel: ["9600", "57600", "115200", "230400", "460800", "921600"]
                        combo: baudRateCombo
                    }

                    SettingsRow {
                        label: "DataBits"
                        comboModel: ["5", "6", "7", "8"]
                        combo: dataBitsCombo
                    }

                    SettingsRow {
                        label: "Parity"
                        comboModel: ["None", "Even", "Odd"]
                        combo: parityCombo
                    }

                    SettingsRow {
                        label: "StopBit"
                        comboModel: ["1", "1.5", "2"]
                        combo: stopBitCombo
                    }
                }
            }
        }
    }

    ComboBox {
        id: baudRateCombo
        visible: false
        model: ["9600", "57600", "115200", "230400", "460800", "921600"]
        currentIndex: 4
    }

    ComboBox {
        id: dataBitsCombo
        visible: false
        model: ["5", "6", "7", "8"]
        currentIndex: 3
    }

    ComboBox {
        id: parityCombo
        visible: false
        model: ["None", "Even", "Odd"]
    }

    ComboBox {
        id: stopBitCombo
        visible: false
        model: ["1", "1.5", "2"]
    }

    component SettingsRow: Row {
        id: settingsRow

        property string label: ""
        property var comboModel: []
        property ComboBox combo

        width: parent.width
        height: 28
        spacing: 12

        Text {
            width: 74
            height: parent.height
            text: settingsRow.label
            color: "white"
            font.pixelSize: 14
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        ComboBox {
            id: valueCombo
            width: parent.width - 74 - parent.spacing
            height: parent.height
            model: settingsRow.comboModel
            currentIndex: settingsRow.combo ? settingsRow.combo.currentIndex : 0
            font.pixelSize: 13
            leftPadding: 8
            rightPadding: 20

            onActivated: function(index) {
                if (settingsRow.combo) {
                    settingsRow.combo.currentIndex = index
                }
            }

            contentItem: Item {
                clip: true

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: valueCombo.leftPadding
                    anchors.rightMargin: valueCombo.rightPadding
                    text: valueCombo.currentText
                    color: "white"
                    font: valueCombo.font
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            indicator: Canvas {
                x: valueCombo.width - width - 7
                y: (valueCombo.height - height) / 2
                width: 10
                height: 7

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    ctx.strokeStyle = "white"
                    ctx.lineWidth = 2
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.beginPath()
                    ctx.moveTo(1, 1)
                    ctx.lineTo(width / 2, height - 1)
                    ctx.lineTo(width - 1, 1)
                    ctx.stroke()
                }
            }

            background: Rectangle {
                radius: 5
                color: "#222222"
                border.color: valueCombo.hovered || valueCombo.activeFocus ? "#4c8bf5" : "#484848"
                border.width: 1
            }

            popup: Popup {
                y: valueCombo.height + 4
                width: valueCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 0

                contentItem: ListView {
                    clip: true
                    implicitHeight: Math.min(contentHeight, 150)
                    model: valueCombo.popup.visible ? valueCombo.delegateModel : null
                    currentIndex: valueCombo.highlightedIndex
                }

                background: Rectangle {
                    color: "#202020"
                    radius: 5
                    border.color: "#3f3f3f"
                }
            }

            delegate: ItemDelegate {
                width: valueCombo.width
                height: 28
                contentItem: Text {
                    text: modelData
                    color: "white"
                    font.pixelSize: 13
                    leftPadding: 8
                    rightPadding: 8
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    color: highlighted ? "#4c8bf5" : "transparent"
                }
            }
        }
    }
}
