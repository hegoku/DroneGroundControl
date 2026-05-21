import QtQuick
import "../components"

Rectangle {
    id: root

    anchors.fill: parent
    color: "#fbfcfd"

    StyledTab {
        anchors.fill: parent
        anchors.margins: 5
        tabs: [
            { "title": "IMU", "source": Qt.resolvedUrl("SensorImuPage.qml") },
            { "title": "Magnetometer", "source": Qt.resolvedUrl("SensorMagnetometerPage.qml") }
        ]
    }
}
