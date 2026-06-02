pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import "../components"

Window {
    id: root

    width: 960
    height: 720
    minimumWidth: 640
    minimumHeight: 480
    visible: false
    title: "Magnetometer 3D Sample Comparison"
    color: "#fbfcfd"

    property var rawPoints: []
    property var calibratedPoints: []

    onVisibleChanged: {
        if (visible) {
            raise()
            requestActivate()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            LegendItem {
                color: "#e09a28"
                label: "Original XYZ"
            }

            LegendItem {
                color: "#2458f2"
                label: "Calibrated XYZ"
            }

            Text {
                Layout.fillWidth: true
                text: "Drag to rotate. Use the mouse wheel to zoom."
                color: "#7b8491"
                font.pixelSize: 12
            }

            StyledButton {
                text: "Reset View"
                styleName: "cancel-button"
                Layout.preferredWidth: 88
                onClicked: pointCloudView.resetView()
            }
        }

        MagnetometerPointCloudView {
            id: pointCloudView

            Layout.fillWidth: true
            Layout.fillHeight: true
            rawPoints: root.rawPoints
            calibratedPoints: root.calibratedPoints
        }
    }

    component LegendItem: RowLayout {
        id: legend

        property color color: "#000000"
        property string label: ""

        spacing: 5

        Rectangle {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10
            radius: 5
            color: legend.color
        }

        Text {
            text: legend.label
            color: "#59616d"
            font.pixelSize: 12
        }
    }
}
