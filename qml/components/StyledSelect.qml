import QtQuick
import QtQuick.Controls

Item {
    id: control

    property var model: []
    property int currentIndex: 0
    property int delegateHeight: 24
    property int maximumPopupHeight: 180
    readonly property int count: model && model.length !== undefined ? model.length : 0
    readonly property string currentText: currentIndex >= 0 && currentIndex < count ? String(model[currentIndex]) : ""
    signal activated(int index)

    implicitWidth: 160
    implicitHeight: 28
    activeFocusOnTab: enabled
    opacity: enabled ? 1.0 : 0.55

    function openPopup() {
        if (enabled && count > 0) {
            popup.open()
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 4
        color: control.enabled ? "#ffffff" : "#fbfcfd"
        border.color: control.activeFocus || mouseArea.containsMouse || popup.visible ? "#bfc7d1" : "#cfd5dd"
        border.width: 1
    }

    Text {
        anchors.left: parent.left
        anchors.right: indicator.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 10
        anchors.rightMargin: 8
        text: control.currentText
        color: control.enabled ? "#2d333b" : "#9aa3af"
        font.pixelSize: 13
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Canvas {
        id: indicator
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        width: 10
        height: 6

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = control.enabled ? "#6b7280" : "#9aa3af"
            ctx.lineWidth = 1.6
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
        id: mouseArea
        anchors.fill: parent
        enabled: control.enabled
        hoverEnabled: true
        cursorShape: control.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: control.openPopup()
    }

    Keys.onReturnPressed: openPopup()
    Keys.onSpacePressed: openPopup()

    Popup {
        id: popup
        x: 0
        y: control.height + 4
        width: control.width
        height: Math.min(control.count * control.delegateHeight, control.maximumPopupHeight)
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Rectangle {
            anchors.fill: parent
            radius: 4
            color: "#ffffff"
            border.color: "#cfd5dd"
            border.width: 1
            clip: true

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: control.model
                currentIndex: control.currentIndex
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    width: listView.width
                    height: control.delegateHeight
                    color: itemArea.containsMouse || index === control.currentIndex ? "#eef4ff" : "#ffffff"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        text: modelData
                        color: "#2d333b"
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    MouseArea {
                        id: itemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            control.currentIndex = index
                            control.activated(index)
                            popup.close()
                        }
                    }
                }
            }
        }
    }
}
