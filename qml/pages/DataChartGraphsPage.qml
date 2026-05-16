import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtGraphs
import DroneGroundControl 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f4f6f8"

    DataChartGraphsAdapter {
        id: graphsAdapter
        model: chartDataModel
        pixelWidth: Math.max(1, Math.round(graphView.width))
    }

    DataChartBenchmarkRunner {
        id: benchmark
        model: chartDataModel
        renderer: "qtgraphs"
        targetRateHz: 2000000
        durationMs: chartBenchmarkDurationMs > 0 ? chartBenchmarkDurationMs : 5000
    }

    function syncGraphs() {
        graphsAdapter.syncAxes(axisX, axisY)
        graphsAdapter.syncSeries(series0, 0)
        graphsAdapter.syncSeries(series1, 1)
        graphsAdapter.syncSeries(series2, 2)
        graphsAdapter.syncSeries(series3, 3)
    }

    Component.onCompleted: {
        benchmark.attachWindow(Window.window)
        chartDataModel.ensureBenchmarkSeries()
        syncGraphs()
        if (chartBenchmarkAutoStart === "qtgraphs") {
            Qt.callLater(benchmark.start)
        }
    }

    Connections {
        target: benchmark

        function onSummaryChanged() {
            if (chartBenchmarkAutoStart === "qtgraphs" && benchmark.summary.length > 0) {
                console.log("DGC_BENCHMARK_RESULT " + benchmark.summary)
                Qt.quit()
            }
        }
    }

    Connections {
        target: chartDataModel

        function onChartChanged() {
            root.syncGraphs()
        }

        function onCountChanged() {
            root.syncGraphs()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 255
            Layout.fillHeight: true
            radius: 8
            color: "#ffffff"
            border.color: "#d9dee5"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    text: "Qt Graphs"
                    color: "#111827"
                    font.pixelSize: 22
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#d9dee5"
                }

                Repeater {
                    model: Math.min(chartDataModel.count, 4)

                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            color: chartDataModel.seriesColor(index)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: chartDataModel.seriesName(index)
                            elide: Text.ElideRight
                            color: "#111827"
                            font.pixelSize: 14
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                Label {
                    Layout.fillWidth: true
                    text: benchmark.running
                          ? "Generated: " + Math.round(benchmark.generatedRateHz).toLocaleString(Qt.locale()) + " Hz\nFPS: " + benchmark.fps.toFixed(1) + "\nCPU: " + benchmark.cpuPercent.toFixed(0) + "%\nRSS: " + benchmark.memoryMb.toFixed(1) + " MB"
                          : (benchmark.summary.length > 0 ? benchmark.summary : "Ready for 2 MHz benchmark")
                    color: "#374151"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    text: benchmark.running ? "Stop Benchmark" : "Run 2 MHz Benchmark"
                    onClicked: benchmark.running ? benchmark.stop() : benchmark.start()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: "#ffffff"
            border.color: "#d9dee5"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    spacing: 10

                    Label {
                        text: "Qt Graphs LineSeries"
                        color: "#111827"
                        font.pixelSize: 22
                        font.bold: true
                    }

                    Label {
                        text: "same DataChartModel backend"
                        color: "#6b7280"
                        font.pixelSize: 13
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Button {
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 34
                        text: "+"
                        onClicked: chartDataModel.zoomIn()
                    }

                    Button {
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 34
                        text: "-"
                        onClicked: chartDataModel.zoomOut()
                    }

                    Button {
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 34
                        text: "D"
                        checkable: true
                        checked: chartDataModel.dragEnabled
                        onClicked: chartDataModel.dragEnabled = !chartDataModel.dragEnabled
                    }
                }

                GraphsView {
                    id: graphView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clipPlotArea: true
                    marginLeft: 56
                    marginRight: 20
                    marginTop: 16
                    marginBottom: 42

                    onWidthChanged: root.syncGraphs()

                    axisX: ValueAxis {
                        id: axisX
                        min: 0
                        max: 1000000
                        labelFormat: "%.6f"
                        tickInterval: 200000
                        subTickCount: 1
                        labelDecimals: 0
                        labelDelegate: Item {
                            property string text

                            implicitWidth: axisXLabel.implicitWidth
                            implicitHeight: axisXLabel.implicitHeight

                            Text {
                                id: axisXLabel
                                anchors.centerIn: parent
                                text: chartDataModel.formatAxisLabel(Number(parent.text))
                                color: "#1f2933"
                                font.pixelSize: 12
                            }
                        }
                    }

                    axisY: ValueAxis {
                        id: axisY
                        min: -1
                        max: 5
                        labelFormat: "%.6f"
                        tickInterval: 1
                        subTickCount: 1
                        labelDecimals: 1
                        labelDelegate: Item {
                            property string text

                            implicitWidth: axisYLabel.implicitWidth
                            implicitHeight: axisYLabel.implicitHeight

                            Text {
                                id: axisYLabel
                                anchors.centerIn: parent
                                text: chartDataModel.formatAxisLabel(Number(parent.text))
                                color: "#1f2933"
                                font.pixelSize: 12
                            }
                        }
                    }

                    theme: GraphsTheme {
                        colorScheme: GraphsTheme.ColorScheme.Light
                        grid.mainColor: "#d9dde3"
                        grid.subColor: "#eef1f4"
                        axisX.mainColor: "#c8cdd4"
                        axisY.mainColor: "#c8cdd4"
                        axisX.labelTextColor: "#1f2933"
                        axisY.labelTextColor: "#1f2933"
                    }

                    LineSeries {
                        id: series0
                        width: 1.4
                    }

                    LineSeries {
                        id: series1
                        width: 1.4
                    }

                    LineSeries {
                        id: series2
                        width: 1.4
                    }

                    LineSeries {
                        id: series3
                        width: 1.4
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: chartDataModel.dragEnabled
                        acceptedButtons: Qt.RightButton
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                        property real lastX: 0

                        onPressed: function(mouse) {
                            if (mouse.button !== Qt.RightButton) {
                                mouse.accepted = false
                                return
                            }
                            lastX = mouse.x
                            mouse.accepted = true
                        }

                        onPositionChanged: function(mouse) {
                            if (!pressed) {
                                return
                            }

                            chartDataModel.panByPixels(mouse.x - lastX,
                                                       Math.max(1, graphView.width - graphView.marginLeft - graphView.marginRight))
                            lastX = mouse.x
                        }
                    }
                }
            }
        }
    }
}
