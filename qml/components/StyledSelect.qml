import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: control

    implicitHeight: 36
    font.pixelSize: 15
    leftPadding: 16
    rightPadding: 36

    contentItem: Text {
        anchors.fill: parent
        anchors.leftMargin: control.leftPadding
        anchors.rightMargin: control.rightPadding
        text: control.currentText
        color: control.enabled ? "#2d333b" : "#9aa3af"
        font: control.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        x: control.width - width - 16
        y: (control.height - height) / 2
        width: 10
        height: 7

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = control.enabled ? "#2d333b" : "#9aa3af"
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
        radius: 4
        color: control.enabled ? "#ffffff" : "#fbfcfd"
        border.color: control.enabled
                      ? (control.hovered || control.activeFocus || control.popup.visible ? "#bfc7d1" : "#cfd5dd")
                      : "#e3e7ec"
        border.width: 1
    }

    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        contentItem: ListView {
            clip: true
            implicitHeight: Math.min(contentHeight, 180)
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
        }

        background: Rectangle {
            radius: 4
            color: "#ffffff"
            border.color: "#cfd5dd"
            border.width: 1
        }
    }

    delegate: ItemDelegate {
        width: control.width
        height: 34

        contentItem: Text {
            text: modelData
            color: "#2d333b"
            font.pixelSize: 15
            leftPadding: 16
            rightPadding: 16
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: highlighted ? "#eef4ff" : "#ffffff"
        }
    }
}
