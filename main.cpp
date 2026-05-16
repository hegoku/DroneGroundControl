#include "src/app/DataFrameTableModel.h"
#include "src/app/DataChartModel.h"
#include "src/app/DataChartView.h"
#include "src/app/ConnectionSession.h"
#include "src/app/Flight.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ConnectionSession connectionSession;
    Flight flight;
    DataFrameTableModel dataTableModel;
    DataChartModel dataChartModel;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("droneSession", &connectionSession);
    engine.rootContext()->setContextProperty("flight", &flight);
    engine.rootContext()->setContextProperty("dataTableModel", &dataTableModel);
    engine.rootContext()->setContextProperty("chartDataModel", &dataChartModel);

    const QString benchmarkRenderer = QString::fromLocal8Bit(qgetenv("DGC_CHART_BENCHMARK")).trimmed().toLower();
    QString initialPageSource;
    if (benchmarkRenderer == QStringLiteral("custom")) {
        initialPageSource = QStringLiteral("pages/DataChartPage.qml");
    } else if (benchmarkRenderer == QStringLiteral("qtgraphs")) {
        initialPageSource = QStringLiteral("pages/DataChartGraphsPage.qml");
    }
    engine.rootContext()->setContextProperty("initialPageSource", initialPageSource);
    engine.rootContext()->setContextProperty("chartBenchmarkAutoStart", benchmarkRenderer);
    engine.rootContext()->setContextProperty("chartBenchmarkDurationMs",
                                             QString::fromLocal8Bit(qgetenv("DGC_CHART_BENCHMARK_MS")).toInt());
    QObject::connect(&connectionSession,
                     &ConnectionSession::telemetryFramesReceived,
                     &dataTableModel,
                     &DataFrameTableModel::processFrames,
                     Qt::QueuedConnection);
    QObject::connect(&connectionSession,
                     &ConnectionSession::telemetryFramesReceived,
                     &dataChartModel,
                     &DataChartModel::processFrames,
                     Qt::QueuedConnection);
    QObject::connect(&dataTableModel,
                     &DataFrameTableModel::addSelectedData,
                     &dataChartModel,
                     &DataChartModel::addSeries);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("DroneGroundControl", "Main");

    return app.exec();
}
