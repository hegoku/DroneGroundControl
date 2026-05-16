#pragma once

#include "DataChartModel.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class DataChartBenchmarkRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DataChartModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString renderer READ renderer WRITE setRenderer NOTIFY rendererChanged)
    Q_PROPERTY(int targetRateHz READ targetRateHz WRITE setTargetRateHz NOTIFY targetRateHzChanged)
    Q_PROPERTY(int durationMs READ durationMs WRITE setDurationMs NOTIFY durationMsChanged)
    Q_PROPERTY(double generatedRateHz READ generatedRateHz NOTIFY metricsChanged)
    Q_PROPERTY(double fps READ fps NOTIFY metricsChanged)
    Q_PROPERTY(double cpuPercent READ cpuPercent NOTIFY metricsChanged)
    Q_PROPERTY(double memoryMb READ memoryMb NOTIFY metricsChanged)
    Q_PROPERTY(quint64 generatedFrames READ generatedFrames NOTIFY metricsChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)

public:
    explicit DataChartBenchmarkRunner(QObject *parent = nullptr);

    DataChartModel *model() const;
    void setModel(DataChartModel *model);

    bool running() const;
    QString renderer() const;
    void setRenderer(const QString &renderer);

    int targetRateHz() const;
    void setTargetRateHz(int rateHz);

    int durationMs() const;
    void setDurationMs(int durationMs);

    double generatedRateHz() const;
    double fps() const;
    double cpuPercent() const;
    double memoryMb() const;
    quint64 generatedFrames() const;
    QString summary() const;

    Q_INVOKABLE void attachWindow(QObject *windowObject);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

signals:
    void modelChanged();
    void runningChanged();
    void rendererChanged();
    void targetRateHzChanged();
    void durationMsChanged();
    void metricsChanged();
    void summaryChanged();

private:
    void feed();
    void sampleMetrics();
    QVector<_un_anotc_v8_frame> makeFrames(int count);
    static _un_anotc_v8_frame makeEulerFrame(quint32 sequence);
    static double processCpuSeconds();
    static double residentMemoryMb();

    QPointer<DataChartModel> m_model;
    QPointer<QQuickWindow> m_window;
    QTimer m_feedTimer;
    QTimer m_metricsTimer;
    QElapsedTimer m_elapsed;
    QString m_renderer = QStringLiteral("custom");
    bool m_running = false;
    int m_targetRateHz = 2000000;
    int m_durationMs = 5000;
    int m_maxBatchFrames = 20000;
    quint64 m_generatedFrames = 0;
    quint32 m_sequence = 0;
    quint64 m_frameSwaps = 0;
    quint64 m_lastFrameSwaps = 0;
    qint64 m_lastMetricsMs = 0;
    double m_lastCpuSeconds = 0.0;
    double m_generatedRateHz = 0.0;
    double m_fps = 0.0;
    double m_cpuPercent = 0.0;
    double m_memoryMb = 0.0;
    QString m_summary;
};
