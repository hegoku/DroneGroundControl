import QtQuick
import QtQuick.Layouts

pragma ComponentBehavior: Bound

Item {
    id: root

    property var tabs: []
    property int currentIndex: 0
    property int tabHeight: 30
    property int tabMinWidth: 150
    property color accentColor: "#4c75f2"

    function tabTitle(index) {
        if (!tabs || index < 0 || index >= tabs.length) {
            return ""
        }
        return String(tabs[index].title || "")
    }

    function tabSource(index) {
        if (!tabs || index < 0 || index >= tabs.length) {
            return ""
        }
        return tabs[index].source || ""
    }

    onTabsChanged: {
        if (!tabs || tabs.length === 0) {
            currentIndex = 0
        } else if (currentIndex >= tabs.length) {
            currentIndex = tabs.length - 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Row {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: root.tabHeight
            spacing: -1
            z: 2

            Repeater {
                model: root.tabs ? root.tabs.length : 0

                delegate: Rectangle {
                    id: tabButton

                    required property int index

                    width: Math.max(root.tabMinWidth, tabLabel.implicitWidth + 40)
                    height: root.tabHeight
                    color: selected ? "#ffffff" : "#fbfcfd"
                    border.color: "#d6dbe2"
                    border.width: 1

                    readonly property bool selected: root.currentIndex === index

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.radius
                        color: parent.color
                    }

                    Text {
                        id: tabLabel
                        anchors.centerIn: parent
                        text: root.tabTitle(tabButton.index)
                        color: tabButton.selected ? root.accentColor : "#59616c"
                        font.pixelSize: 13
                        font.bold: tabButton.selected
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 2
                        color: root.accentColor
                        visible: tabButton.selected
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.currentIndex = tabButton.index
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            clip: true

            Loader {
                id: pageLoader
                anchors.fill: parent
                anchors.topMargin: 5
                source: root.tabSource(root.currentIndex)
            }
        }
    }
}
