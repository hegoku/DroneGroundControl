import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f4f6f8"

    property var columnWidths: [110, 240, 150, 130, 300]
    property var minimumColumnWidths: [70, 120, 90, 80, 140]
    property real lastTableWidth: 0

    function columnWidth(column) {
        return columnWidths[column]
    }

    function totalColumnWidth(widths) {
        var total = 0
        for (var i = 0; i < widths.length; ++i) {
            total += widths[i]
        }
        return total
    }

    function setDefaultColumnWidths(width) {
        var proportions = [0.13, 0.26, 0.15, 0.13, 0.33]
        var widths = []
        var used = 0
        for (var i = 0; i < proportions.length - 1; ++i) {
            widths[i] = Math.max(minimumColumnWidths[i], Math.round(width * proportions[i]))
            used += widths[i]
        }
        widths[4] = Math.max(minimumColumnWidths[4], width - used)
        columnWidths = widths
        lastTableWidth = width
    }

    function fitColumnsToWidth(width) {
        if (width <= 0) {
            return
        }

        if (lastTableWidth <= 0) {
            setDefaultColumnWidths(width)
            return
        }

        var widths = columnWidths.slice()
        var delta = width - totalColumnWidth(widths)
        if (delta >= 0) {
            widths[4] += delta
        } else {
            for (var i = widths.length - 1; i >= 0 && delta < 0; --i) {
                var shrink = Math.min(widths[i] - minimumColumnWidths[i], -delta)
                widths[i] -= shrink
                delta += shrink
            }
        }

        columnWidths = widths
        lastTableWidth = width
    }

    function resizeColumn(column, delta) {
        if (column < 0 || column >= columnWidths.length - 1) {
            return
        }

        var widths = columnWidths.slice()
        var left = widths[column]
        var right = widths[column + 1]
        var clampedDelta = Math.max(minimumColumnWidths[column] - left,
                                    Math.min(delta, right - minimumColumnWidths[column + 1]))

        if (clampedDelta === 0) {
            return
        }

        widths[column] = left + clampedDelta
        widths[column + 1] = right - clampedDelta
        columnWidths = widths
    }

    Rectangle {
        id: tableFrame
        anchors.fill: parent
        anchors.margins: 18
        color: "#ffffff"
        border.color: "#c7cdd4"
        border.width: 1
        clip: true

        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 36
            color: "#edf0f4"
            border.color: "#d9dee5"
            border.width: 1

            Row {
                anchors.fill: parent

                HeaderCell {
                    width: root.columnWidth(0)
                    text: "ID"
                    resizeColumn: 0
                }
                HeaderCell {
                    width: root.columnWidth(1)
                    text: "Name"
                    resizeColumn: 1
                }
                HeaderCell {
                    width: root.columnWidth(2)
                    text: "Value"
                    resizeColumn: 2
                }
                HeaderCell {
                    width: root.columnWidth(3)
                    text: "Type"
                    resizeColumn: 3
                }
                HeaderCell {
                    width: root.columnWidth(4)
                    text: "Description"
                    showDivider: false
                }
            }
        }

        ListView {
            id: tableView
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            model: dataTableModel
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 1200
            reuseItems: true

            onWidthChanged: root.fitColumnsToWidth(width)

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: rowItem
                width: tableView.width
                height: 32
                color: hoverHandler.hovered ? "#eef4ff" : (index % 2 === 0 ? "#ffffff" : "#fbfcfd")

                Row {
                    anchors.fill: parent

                    Item {
                        width: root.columnWidth(0)
                        height: parent.height

                        Image {
                            id: disclosure
                            x: 10
                            width: 13
                            height: 13
                            anchors.verticalCenter: parent.verticalCenter
                            visible: model.expandable
                            source: model.expandable
                                    ? (model.expanded
                                       ? "qrc:/resources/icons/chevron-down.svg"
                                       : "qrc:/resources/icons/chevron-right.svg")
                                    : ""
                            opacity: 0.55
                            sourceSize.width: 13
                            sourceSize.height: 13
                            fillMode: Image.PreserveAspectFit
                        }

                        Text {
                            anchors.left: disclosure.right
                            anchors.right: parent.right
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.idText
                            color: "#15181c"
                            font.pixelSize: 16
                            elide: Text.ElideRight
                        }
                    }

                    BodyCell {
                        width: root.columnWidth(1)
                        text: model.nameText
                        leftPadding: model.depth === 0 ? 10 : 18
                        strong: model.depth === 0
                    }
                    BodyCell {
                        width: root.columnWidth(2)
                        text: model.valueText
                        textColor: model.valueText.length > 0 ? "#0f5132" : "#6b7280"
                    }
                    BodyCell {
                        width: root.columnWidth(3)
                        text: model.typeText
                    }
                    BodyCell {
                        width: root.columnWidth(4)
                        text: model.descriptionText
                    }
                }

                HoverHandler {
                    id: hoverHandler
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: model.expandable ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            rowMenu.row = index
                            rowMenu.popup(rowItem, mouse.x, mouse.y)
                        } else if (model.expandable) {
                            dataTableModel.toggleExpanded(index)
                        }
                    }
                }
            }
        }

        Menu {
            id: rowMenu
            property int row: -1

            MenuItem {
                text: "Add to Data Chart"
                onTriggered: dataTableModel.addToDataChart(rowMenu.row)
            }
        }
    }

    component HeaderCell: Item {
        property alias text: title.text
        property bool showDivider: true
        property int resizeColumn: -1

        height: parent.height

        Text {
            id: title
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: "#262a31"
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }

        Rectangle {
            id: divider
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: 5
            anchors.bottomMargin: 5
            width: 1
            color: "#c9ced6"
            visible: showDivider
        }

        MouseArea {
            id: resizeArea
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 10
            enabled: showDivider && resizeColumn >= 0
            hoverEnabled: true
            cursorShape: Qt.SplitHCursor

            property real lastHeaderX: 0

            onPressed: function(mouse) {
                lastHeaderX = mapToItem(header, mouse.x, mouse.y).x
            }

            onPositionChanged: function(mouse) {
                if (!pressed) {
                    return
                }

                var headerX = mapToItem(header, mouse.x, mouse.y).x
                root.resizeColumn(resizeColumn, headerX - lastHeaderX)
                lastHeaderX = headerX
            }

            Rectangle {
                anchors.centerIn: parent
                width: 2
                height: parent.height
                color: resizeArea.pressed || resizeArea.containsMouse ? "#4c8bf5" : divider.color
                opacity: resizeArea.pressed || resizeArea.containsMouse ? 1 : 0
            }
        }
    }

    component BodyCell: Item {
        property alias text: value.text
        property color textColor: "#111827"
        property int leftPadding: 10
        property bool strong: false

        height: parent.height

        Text {
            id: value
            anchors.left: parent.left
            anchors.leftMargin: leftPadding
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            color: textColor
            font.pixelSize: 16
            font.bold: strong
            elide: Text.ElideRight
        }
    }
}
