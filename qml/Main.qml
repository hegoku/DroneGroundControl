import QtQuick
import QtQuick.Controls

import "./components"

ApplicationWindow {
    width: 1200
    height: 700
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: qsTr("DroneGroundControl")
    property string startPage: initialPageSource && initialPageSource.length > 0
                               ? initialPageSource
                               : "pages/HomePage.qml"

    header:Rectangle {
        width: parent.width
        height: 60
        color: "#1e1e1e"

        Item {
            id: topBar
            property string connectionMode: "serial"
            property bool connectionBusy: false

            function disconnectAfterDeviceInfo() {
                if (connectionBusy)
                    return

                connectionBusy = true
                droneSession.closeWithDisconnectCommand()
            }

            function connectSerial() {
                if (connectionBusy)
                    return

                connectionBusy = true
                var opened = droneSession.openSerial(serialConnection.portName,
                                                     Number(serialConnection.baudRate),
                                                     Number(serialConnection.dataBits),
                                                     serialConnection.parity,
                                                     serialConnection.stopBit)
                if (!opened)
                    connectionBusy = false
            }

            function connectUdp() {
                if (connectionBusy)
                    return

                connectionBusy = true
                var opened = droneSession.openUdp(udpConnection.host,
                                                  Number(udpConnection.remotePort),
                                                  Number(udpConnection.localListenPort))
                if (!opened)
                    connectionBusy = false
            }

            anchors.centerIn: parent
            height: parent.height
            width: parent.width

            ConnectionButton {
                id: connectionBtn
                on: droneSession.isOpen || topBar.connectionBusy
                onClicked: {
                    if (topBar.connectionBusy) {
                        return
                    } else if (droneSession.isOpen) {
                        topBar.disconnectAfterDeviceInfo()
                        flight.reset()
                    } else if (topBar.connectionMode === "serial") {
                        topBar.connectSerial()
                    } else if (topBar.connectionMode === "udp") {
                        topBar.connectUdp()
                    }
                }
                height: parent.height
                width: parent.height
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                id: connectionControls
                anchors.left: connectionBtn.right
                anchors.leftMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                height: 34
                spacing: 18

                Rectangle {
                    id: connectionTypeSwitch
                    width: 126
                    height: 32
                    radius: 5
                    color: droneSession.isOpen ? "#202020" : "#252525"
                    border.color: "#3a3a3a"
                    border.width: 1
                    opacity: droneSession.isOpen ? 0.55 : 1.0
                    clip: true

                    Row {
                        anchors.fill: parent

                        Repeater {
                            model: ["Serial", "UDP"]

                            delegate: Rectangle {
                                width: connectionTypeSwitch.width / 2
                                height: connectionTypeSwitch.height
                                radius: 5
                                color: selected ? "#4c8bf5" : "transparent"
                                border.color: selected ? "#6fa4ff" : "transparent"
                                border.width: 1

                                property bool selected: topBar.connectionMode === modelData.toLowerCase()

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: parent.selected
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !droneSession.isOpen
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: topBar.connectionMode = modelData.toLowerCase()
                                }
                            }
                        }
                    }
                }

                SerialPortConnection {
                    id: serialConnection
                    visible: topBar.connectionMode === "serial"
                    enabled: !droneSession.isOpen
                    opacity: enabled ? 1.0 : 0.55
                    anchors.verticalCenter: parent.verticalCenter

                    function refreshAvailablePorts() {
                        var ports = droneSession.availableSerialPorts()
                        availablePorts = ports
                    }

                    Component.onCompleted: refreshAvailablePorts()
                    onRefreshRequested: refreshAvailablePorts()
                }

                UdpConnection {
                    id: udpConnection
                    visible: topBar.connectionMode === "udp"
                    enabled: !droneSession.isOpen
                    opacity: enabled ? 1.0 : 0.55
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                anchors.right: parent.right
                height: parent.height
                Battery {
                    id: batteryWidget
                    level: 0.65
                    charging: false
                    width: 80
                    height: 40
                    anchors.verticalCenter: parent.verticalCenter
                }

                SensorStatus {
                    id: imuStatus
                    label: "IMU"
                    on: flight.imu
                    onImg: "qrc:/resources/icons/sensor_gyro_on.png"
                    offImg: "qrc:/resources/icons/sensor_gyro_off.png"
                    height: parent.height
                    width: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                }

                SensorStatus {
                    id: magStatus
                    label: "Mag"
                    on: flight.mag
                    onImg: "qrc:/resources/icons/sensor_mag_on.png"
                    offImg: "qrc:/resources/icons/sensor_mag_off.png"
                    height: parent.height
                    width: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                }

                SensorStatus {
                    id: baroStatus
                    label: "Baro"
                    on: flight.baro
                    onImg: "qrc:/resources/icons/sensor_baro_on.png"
                    offImg: "qrc:/resources/icons/sensor_baro_off.png"
                    height: parent.height
                    width: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Connections {
                target: droneSession

                function onErrorOccurred(message) {
                    topBar.connectionBusy = false
                    console.warn("Connection error:", message)
                }

                function onLog(message) {
                    console.log("Connection:", message)
                }

                function onIsOpenChanged() {
                    if (!droneSession.isOpen) {
                        topBar.connectionBusy = false
                        flight.reset()
                        return
                    }

                    topBar.connectionBusy = false
                    droneSession.requestDeviceInfo(0x01, function(result) {
                        flight.imu = result.frame.imu
                        flight.mag = result.frame.mag
                        flight.baro = result.frame.baro
                    }, function(error) {
                        console.warn("Device info request failed:", error.reason)
                    })
                }

                function onRequestStarted(requestId, name) {
                    console.log("Request started:", requestId, name)
                }

                function onRequestSucceeded(requestId, name) {
                    console.log("Request succeeded:", requestId, name)
                }

                function onRequestFailed(requestId, name, reason) {
                    console.warn("Request failed:", requestId, name, reason)
                }
            }
        }
    }

    Item {
        width: parent.width
        height: parent.height

        Rectangle {
            id: menuPanel
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 150
            color: "#2A2A2A"

            ListView {
                id: menuList
                anchors.fill: parent
                spacing: 6
                clip: true

                model: ListModel {
                    ListElement { name: "Home";    page: "pages/HomePage.qml" }
                    ListElement { name: "Parameters";    page: "pages/ParameterPage.qml" }
                    ListElement { name: "Data Frame";    page: "pages/DataFrameTablePage.qml" }
                    ListElement { name: "Data Analysis";    page: "pages/DataChartPage.qml" }
                    ListElement { name: "Qt Graphs";    page: "pages/DataChartGraphsPage.qml" }
                    ListElement { name: "RC";    page: "pages/RC.qml" }
                }

                delegate: Item {
                    id: menuItem
                    width: parent.width
                    height: 50

                    property bool selected: ListView.isCurrentItem

                    Rectangle {
                        id: bgRect
                        anchors.fill: parent
                        color: selected ? "#4C8BF5" : "transparent"
                    }

                    Text {
                        text: name
                        anchors.centerIn: parent
                        font.pixelSize: 18
                        color: selected ? "white" : "#CCCCCC"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            menuList.currentIndex = index
                            pageLoader.source = Qt.resolvedUrl(page)
                        }
                    }
                }
            }
        }

        Loader {
            id: pageLoader
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: menuPanel.right
            anchors.right: parent.right
            source: startPage
        }

        Connections {
            target: dataTableModel

            function onAddSelectedData(functionId, parameterIndex) {
                menuList.currentIndex = 3
                pageLoader.source = Qt.resolvedUrl("pages/DataChartPage.qml")
            }
        }
    }

}
