import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

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
                    voltage: flight.voltage
                    battery_count: flight.battery_count
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
                    ListElement { name: "Home"; page: "pages/HomePage.qml"; iconSource: "qrc:/resources/icons/home.svg" }
                    ListElement { name: "Sensor"; page: "pages/SensorPage.qml"; iconSource: "qrc:/resources/icons/sensor.svg" }
                    ListElement { name: "Parameters"; page: "pages/ParameterPage.qml"; iconSource: "qrc:/resources/icons/basic-setting.svg" }
                    ListElement { name: "Data Frame"; page: "pages/DataFrameTablePage.qml"; iconSource: "qrc:/resources/icons/interface-ui-table-calendar.svg" }
                    ListElement { name: "Data Analysis"; page: "pages/DataChartPage.qml"; iconSource: "qrc:/resources/icons/chart.svg" }
                    ListElement { name: "RC"; page: "pages/RC.qml"; iconSource: "qrc:/resources/icons/game.svg" }
                    ListElement { name: "Mixer"; page: "pages/MixerPage.qml"; iconSource: "qrc:/resources/icons/drone-mixer.svg" }
                }

                delegate: Item {
                    id: menuItem
                    width: parent.width
                    height: 50
                    activeFocusOnTab: true

                    property bool selected: ListView.isCurrentItem
                    readonly property int leftPadding: 18
                    readonly property int rightPadding: 10
                    readonly property int iconSize: 20
                    readonly property int itemSpacing: 10

                    function activate() {
                        menuList.currentIndex = index
                        pageLoader.source = Qt.resolvedUrl(page)
                    }

                    Accessible.role: Accessible.Button
                    Accessible.name: name

                    Rectangle {
                        anchors.fill: parent
                        color: selected ? "#4C8BF5" : "transparent"
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: menuItem.leftPadding
                        anchors.right: parent.right
                        anchors.rightMargin: menuItem.rightPadding
                        anchors.verticalCenter: parent.verticalCenter
                        height: parent.height
                        spacing: menuItem.itemSpacing

                        IconImage {
                            source: iconSource
                            color: "white"
                            sourceSize.width: menuItem.iconSize
                            sourceSize.height: menuItem.iconSize
                            width: menuItem.iconSize
                            height: menuItem.iconSize
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: name
                            color: selected ? "white" : "#CCCCCC"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            width: parent.width - menuItem.iconSize - parent.spacing
                            height: parent.height
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: menuItem.activate()
                    }

                    Keys.onReturnPressed: activate()
                    Keys.onSpacePressed: activate()

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
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
                menuList.currentIndex = 4
                pageLoader.source = Qt.resolvedUrl("pages/DataChartPage.qml")
            }
        }
    }

}
