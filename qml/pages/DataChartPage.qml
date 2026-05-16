import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import DroneGroundControl 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f4f6f8"

    function buttonColor(checked) {
        return checked ? "#e8f0ff" : "#ffffff"
    }

    function buttonBorder(checked) {
        return checked ? "#4c8bf5" : "#d8dde5"
    }

    FileDialog {
        id: saveDialog
        title: "Save Chart Data"
        fileMode: FileDialog.SaveFile
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        defaultSuffix: "csv"
        onAccepted: chartDataModel.saveCsv(selectedFile)
    }

    FileDialog {
        id: openDialog
        title: "Open Chart CSV"
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            csvWindowComponent.createObject(root, {
                "sourceFile": selectedFile,
                "visible": true
            })
        }
    }

    DataChartBenchmarkRunner {
        id: benchmark
        model: chartDataModel
        renderer: "custom"
        targetRateHz: 2000000
        durationMs: chartBenchmarkDurationMs > 0 ? chartBenchmarkDurationMs : 5000
    }

    Component.onCompleted: {
        benchmark.attachWindow(Window.window)
        if (chartBenchmarkAutoStart === "custom") {
            Qt.callLater(benchmark.start)
        }
    }

    Connections {
        target: benchmark

        function onSummaryChanged() {
            if (chartBenchmarkAutoStart === "custom" && benchmark.summary.length > 0) {
                console.log("DGC_BENCHMARK_RESULT " + benchmark.summary)
                Qt.quit()
            }
        }
    }

    Component {
        id: csvWindowComponent

        Window {
            id: csvWindow
            width: 1060
            height: 680
            minimumWidth: 760
            minimumHeight: 460
            title: "CSV Data Chart"
            property url sourceFile

            DataChartModel {
                id: csvChartModel
                receiving: false
                autoScroll: true
            }

            Component.onCompleted: {
                if (sourceFile.toString().length > 0) {
                    csvChartModel.loadCsv(sourceFile)
                }
                raise()
            }

            Rectangle {
                anchors.fill: parent
                color: "#f4f6f8"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    SignalPanel {
                        Layout.preferredWidth: 250
                        Layout.fillHeight: true
                        chartModel: csvChartModel
                        showAddButton: false
                        showDeleteAllButton: false
                        showRowDeleteButtons: false
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#ffffff"
                        border.color: "#d9dee5"
                        border.width: 1

                        ChartViewPanel {
                            anchors.fill: parent
                            anchors.margins: 10
                            chartModel: csvChartModel
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        SignalPanel {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            chartModel: chartDataModel
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            border.color: "#d9dee5"
            border.width: 1

            ChartViewPanel {
                anchors.fill: parent
                anchors.margins: 5
                chartModel: chartDataModel
                benchmarkRunner: benchmark
                showBenchmarkStatus: true
                showReceiveButton: true
                showAutoScrollButton: true
                showClearButton: true
                showSaveButton: true
                showOpenButton: true
                showBenchmarkButton: true
                onSaveRequested: saveDialog.open()
                onOpenRequested: openDialog.open()
            }
        }
    }

    component ChartViewPanel: ColumnLayout {
        id: chartPanel
        property var chartModel
        property var benchmarkRunner
        property bool showBenchmarkStatus: false
        property bool showReceiveButton: false
        property bool showAutoScrollButton: false
        property bool showDragButton: true
        property bool showCursorButton: true
        property bool showCropButton: true
        property bool showClearButton: false
        property bool showSaveButton: false
        property bool showOpenButton: false
        property bool showBenchmarkButton: false
        signal saveRequested()
        signal openRequested()

        spacing: 1

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 25
            spacing: 1

            Label {
                visible: chartPanel.showBenchmarkStatus
                text: chartPanel.benchmarkRunner && chartPanel.benchmarkRunner.running
                      ? "BENCH " + Math.round(chartPanel.benchmarkRunner.generatedRateHz / 1000) + "kHz  " + chartPanel.benchmarkRunner.fps.toFixed(1) + " FPS  " + chartPanel.benchmarkRunner.cpuPercent.toFixed(0) + "% CPU"
                      : (chartPanel.benchmarkRunner ? chartPanel.benchmarkRunner.summary : "")
                color: "#4b5563"
                font.pixelSize: 12
                elide: Text.ElideRight
                Layout.maximumWidth: 360
            }

            Item {
                Layout.fillWidth: true
            }

            ChartToolButton {
                visible: chartPanel.showReceiveButton
                iconSource: chartPanel.chartModel && chartPanel.chartModel.receiving ? "qrc:/resources/icons/pauze.svg" : "qrc:/resources/icons/play.svg"
                tooltip: chartPanel.chartModel && chartPanel.chartModel.receiving ? "Pause receiving chart data" : "Start receiving chart data"
                checked: chartPanel.chartModel && chartPanel.chartModel.receiving
                onClicked: chartPanel.chartModel.receiving = !chartPanel.chartModel.receiving
            }

            ChartToolButton {
                visible: chartPanel.showAutoScrollButton
                text: "A"
                tooltip: "Auto scroll"
                checked: chartPanel.chartModel && chartPanel.chartModel.autoScroll
                onClicked: chartPanel.chartModel.autoScroll = !chartPanel.chartModel.autoScroll
            }

            ChartToolButton {
                visible: chartPanel.showDragButton
                iconSource: "qrc:/resources/icons/drag-arrow.svg"
                tooltip: "Drag chart"
                checked: chartPanel.chartModel && chartPanel.chartModel.dragEnabled
                onClicked: chartPanel.chartModel.dragEnabled = !chartPanel.chartModel.dragEnabled
            }

            ChartToolButton {
                visible: chartPanel.showCursorButton
                iconSource: "qrc:/resources/icons/target.svg"
                tooltip: "Cursor"
                checked: chartPanel.chartModel && chartPanel.chartModel.cursorEnabled
                onClicked: chartPanel.chartModel.cursorEnabled = !chartPanel.chartModel.cursorEnabled
            }

            ChartToolButton {
                visible: chartPanel.showCropButton
                iconSource: "qrc:/resources/icons/crop.svg"
                tooltip: "Crop"
                checked: chartPanel.chartModel && chartPanel.chartModel.cropEnabled
                onClicked: chartPanel.chartModel.cropEnabled = !chartPanel.chartModel.cropEnabled
            }

            ChartToolButton {
                visible: chartPanel.showClearButton
                text: "C"
                tooltip: "Clean up chart"
                onClicked: chartPanel.chartModel.clearData()
            }

            ChartToolButton {
                visible: chartPanel.showSaveButton
                iconSource: "qrc:/resources/icons/save.svg"
                tooltip: "Save chart data to CSV"
                onClicked: chartPanel.saveRequested()
            }

            ChartToolButton {
                visible: chartPanel.showOpenButton
                iconSource: "qrc:/resources/icons/folder.svg"
                tooltip: "Open CSV in new chart window"
                onClicked: chartPanel.openRequested()
            }

            ChartToolButton {
                visible: chartPanel.showBenchmarkButton
                text: "B"
                tooltip: "Run 2 MHz renderer benchmark"
                checked: chartPanel.benchmarkRunner && chartPanel.benchmarkRunner.running
                onClicked: chartPanel.benchmarkRunner.running ? chartPanel.benchmarkRunner.stop() : chartPanel.benchmarkRunner.start()
            }
        }

        DataChartView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: chartPanel.chartModel
        }
    }

    component ChartToolButton: Button {
        id: control
        property string tooltip: ""
        property url iconSource: ""

        Layout.preferredWidth: 22
        Layout.preferredHeight: 22
        checkable: false
        font.pixelSize: 16
        font.bold: true
        padding: 0
        hoverEnabled: true
        palette.buttonText: control.checked ? "#1d4ed8" : "#111827"

        ToolTip.visible: hovered && tooltip.length > 0
        ToolTip.text: tooltip
        ToolTip.delay: 450

        icon.source: iconSource
        icon.width: 18
        icon.height: 18
        icon.color: control.checked ? "#1d4ed8" : "#111827"

        display: iconSource.toString().length > 0
            ? AbstractButton.IconOnly
            : AbstractButton.TextOnly

        // contentItem: Item {
        //     Image {
        //         anchors.centerIn: parent
        //         width: 16
        //         height: 16
        //         source: control.iconSource
        //         sourceSize.width: 16
        //         sourceSize.height: 16
        //         visible: control.iconSource.toString().length > 0
        //         fillMode: Image.PreserveAspectFit
        //     }

        //     Text {
        //         anchors.fill: parent
        //         text: control.iconSource.toString().length > 0 ? "" : control.text
        //         color: control.checked ? "#1d4ed8" : "#111827"
        //         font: control.font
        //         horizontalAlignment: Text.AlignHCenter
        //         verticalAlignment: Text.AlignVCenter
        //     }
        // }

        background: Rectangle {
            radius: 4
            color: control.pressed ? "#eef2f7" : (control.hovered ? "#f7f9fc" : "transparent")
            border.color: control.hovered ? "#cfd6df" : "transparent"
            border.width: 1
        }
    }

    component SignalPanel: Rectangle {
        id: panel
        property var chartModel
        property bool anySignalVisible: false
        property bool showAddButton: true
        property bool showDeleteAllButton: true
        property bool showRowDeleteButtons: true
        signal addSignalRequested()

        function updateVisibilityState() {
            if (!chartModel) {
                anySignalVisible = false
                return
            }

            var count = chartModel.seriesCount()
            var hasVisibleSignal = false
            for (var row = 0; row < count; ++row) {
                if (chartModel.seriesVisible(row)) {
                    hasVisibleSignal = true
                    break
                }
            }
            anySignalVisible = hasVisibleSignal
        }

        function removeAllSignals() {
            if (!chartModel) {
                return
            }

            for (var row = chartModel.seriesCount() - 1; row >= 0; --row) {
                chartModel.removeSeries(row)
            }
            updateVisibilityState()
        }

        onChartModelChanged: updateVisibilityState()

        Component.onCompleted: updateVisibilityState()

        Connections {
            target: panel.chartModel

            function onChartChanged() {
                panel.updateVisibilityState()
            }

            function onCountChanged() {
                panel.updateVisibilityState()
            }
        }

        color: "#ffffff"
        border.color: "#d9dee5"
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            ListView {
                id: signalList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: panel.chartModel
                spacing: 0

                delegate: Rectangle {
                    width: signalList.width
                    height: 30
                    color: "transparent"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 1
                        color: "#edf0f4"
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            color: model.visible ? model.color : "#ffffff"
                            border.color: model.color
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: panel.chartModel.setSeriesVisible(index, !model.visible)
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: model.name
                            color: "#111827"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.preferredWidth: 56
                            text: model.value.length > 0 ? model.value : "0.00"
                            color: "#111827"
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignRight
                            elide: Text.ElideRight
                        }

                        Button {
                            visible: panel.showRowDeleteButtons
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            padding: 0
                            ToolTip.visible: hovered
                            ToolTip.text: "Remove signal"
                            contentItem: Image {
                                source: "qrc:/resources/icons/cross.svg"
                                sourceSize.width: 10
                                sourceSize.height: 10
                                fillMode: Image.PreserveAspectFit
                            }
                            background: Rectangle {
                                radius: 4
                                color: "transparent"
                                border.color: "transparent"
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: panel.chartModel.removeSeries(index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 22
                spacing: 4

                Item {
                    Layout.fillWidth: true
                }

                SignalActionButton {
                    visible: panel.showAddButton
                    iconSource: "qrc:/resources/icons/plus.svg"
                    tooltip: "Add signal"
                    onClicked: panel.addSignalRequested()
                }

                SignalActionButton {
                    iconSource: panel.anySignalVisible ? "qrc:/resources/icons/hide.svg" : "qrc:/resources/icons/view.svg"
                    tooltip: panel.anySignalVisible ? "Hide all signals" : "Show all signals"
                    onClicked: {
                        if (panel.anySignalVisible) {
                            panel.chartModel.hideAll()
                        } else {
                            panel.chartModel.showAll()
                        }
                        panel.updateVisibilityState()
                    }
                }

                SignalActionButton {
                    visible: panel.showDeleteAllButton
                    iconSource: "qrc:/resources/icons/trash.svg"
                    tooltip: "Delete all signals"
                    onClicked: panel.removeAllSignals()
                }
            }
        }
    }

    component SignalActionButton: Button {
        id: actionButton
        property url iconSource: ""
        property string tooltip: ""

        Layout.preferredWidth: 22
        Layout.preferredHeight: 22
        padding: 0
        hoverEnabled: true

        ToolTip.visible: hovered && tooltip.length > 0
        ToolTip.text: tooltip
        ToolTip.delay: 450

        contentItem: Image {
            source: actionButton.iconSource
            sourceSize.width: 12
            sourceSize.height: 12
            fillMode: Image.PreserveAspectFit
        }

        background: Rectangle {
            radius: 4
            color: actionButton.pressed ? "#eef2f7" : (actionButton.hovered ? "#f7f9fc" : "transparent")
            border.color: actionButton.hovered ? "#cfd6df" : "transparent"
            border.width: 1
        }
    }
}
