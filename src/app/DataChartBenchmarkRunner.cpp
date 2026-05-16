#include "DataChartBenchmarkRunner.h"

#include <QDateTime>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <sys/resource.h>

#ifdef Q_OS_DARWIN
#include <mach/mach.h>
#endif

namespace {
constexpr int EulerPayloadSize = 7;

void appendChecksum(_un_anotc_v8_frame *frame)
{
    quint8 sum = 0;
    quint8 add = 0;
    const int size = ANOTC_V8_HEAD_SIZE + frame->frame.len;
    for (int i = 0; i < size; ++i) {
        sum = static_cast<quint8>(sum + frame->rawBytes[i]);
        add = static_cast<quint8>(add + sum);
    }
    frame->rawBytes[size] = sum;
    frame->rawBytes[size + 1] = add;
}
}

DataChartBenchmarkRunner::DataChartBenchmarkRunner(QObject *parent)
    : QObject(parent)
{
    m_feedTimer.setInterval(0);
    connect(&m_feedTimer, &QTimer::timeout, this, &DataChartBenchmarkRunner::feed);

    m_metricsTimer.setInterval(500);
    connect(&m_metricsTimer, &QTimer::timeout, this, &DataChartBenchmarkRunner::sampleMetrics);
}

DataChartModel *DataChartBenchmarkRunner::model() const
{
    return m_model;
}

void DataChartBenchmarkRunner::setModel(DataChartModel *model)
{
    if (m_model == model) {
        return;
    }
    m_model = model;
    emit modelChanged();
}

bool DataChartBenchmarkRunner::running() const
{
    return m_running;
}

QString DataChartBenchmarkRunner::renderer() const
{
    return m_renderer;
}

void DataChartBenchmarkRunner::setRenderer(const QString &renderer)
{
    if (m_renderer == renderer) {
        return;
    }
    m_renderer = renderer;
    emit rendererChanged();
}

int DataChartBenchmarkRunner::targetRateHz() const
{
    return m_targetRateHz;
}

void DataChartBenchmarkRunner::setTargetRateHz(int rateHz)
{
    const int bounded = std::clamp(rateHz, 1, 5000000);
    if (m_targetRateHz == bounded) {
        return;
    }
    m_targetRateHz = bounded;
    emit targetRateHzChanged();
}

int DataChartBenchmarkRunner::durationMs() const
{
    return m_durationMs;
}

void DataChartBenchmarkRunner::setDurationMs(int durationMs)
{
    const int bounded = std::clamp(durationMs, 1000, 120000);
    if (m_durationMs == bounded) {
        return;
    }
    m_durationMs = bounded;
    emit durationMsChanged();
}

double DataChartBenchmarkRunner::generatedRateHz() const
{
    return m_generatedRateHz;
}

double DataChartBenchmarkRunner::fps() const
{
    return m_fps;
}

double DataChartBenchmarkRunner::cpuPercent() const
{
    return m_cpuPercent;
}

double DataChartBenchmarkRunner::memoryMb() const
{
    return m_memoryMb;
}

quint64 DataChartBenchmarkRunner::generatedFrames() const
{
    return m_generatedFrames;
}

QString DataChartBenchmarkRunner::summary() const
{
    return m_summary;
}

void DataChartBenchmarkRunner::attachWindow(QObject *windowObject)
{
    auto *window = qobject_cast<QQuickWindow *>(windowObject);
    if (!window || m_window == window) {
        return;
    }

    if (m_window) {
        disconnect(m_window, nullptr, this, nullptr);
    }

    m_window = window;
    connect(m_window, &QQuickWindow::frameSwapped, this, [this]() {
        ++m_frameSwaps;
    });
}

void DataChartBenchmarkRunner::start()
{
    if (m_running || !m_model) {
        return;
    }

    m_model->ensureBenchmarkSeries();
    m_model->clearData();
    m_model->setReceiving(true);
    m_model->setAutoScroll(true);

    m_running = true;
    m_generatedFrames = 0;
    m_sequence = 0;
    m_frameSwaps = 0;
    m_lastFrameSwaps = 0;
    m_generatedRateHz = 0.0;
    m_fps = 0.0;
    m_cpuPercent = 0.0;
    m_memoryMb = residentMemoryMb();
    m_lastCpuSeconds = processCpuSeconds();
    m_lastMetricsMs = 0;
    m_summary.clear();
    m_elapsed.restart();
    m_feedTimer.start();
    m_metricsTimer.start();
    emit runningChanged();
    emit metricsChanged();
    emit summaryChanged();
}

void DataChartBenchmarkRunner::stop()
{
    if (!m_running) {
        return;
    }

    sampleMetrics();
    m_feedTimer.stop();
    m_metricsTimer.stop();
    m_running = false;
    m_summary = QStringLiteral("%1: %2 Hz generated, %3 FPS, %4% CPU, %5 MB RSS, %6 frames")
                    .arg(m_renderer,
                         QString::number(m_generatedRateHz, 'f', 0),
                         QString::number(m_fps, 'f', 1),
                         QString::number(m_cpuPercent, 'f', 1),
                         QString::number(m_memoryMb, 'f', 1),
                         QString::number(m_generatedFrames));
    emit runningChanged();
    emit summaryChanged();
}

void DataChartBenchmarkRunner::feed()
{
    if (!m_running || !m_model) {
        return;
    }

    const qint64 elapsedMs = m_elapsed.elapsed();
    if (elapsedMs >= m_durationMs) {
        stop();
        return;
    }

    const quint64 targetFrames = static_cast<quint64>((static_cast<double>(elapsedMs) / 1000.0) * m_targetRateHz);
    quint64 due = targetFrames > m_generatedFrames ? targetFrames - m_generatedFrames : 1;
    due = std::min<quint64>(due, m_maxBatchFrames);

    QVector<_un_anotc_v8_frame> frames = makeFrames(static_cast<int>(due));
    m_model->processFrames(frames);
    m_generatedFrames += static_cast<quint64>(frames.size());
}

void DataChartBenchmarkRunner::sampleMetrics()
{
    const qint64 elapsedMs = std::max<qint64>(1, m_elapsed.elapsed());
    const qint64 deltaMs = m_lastMetricsMs > 0 ? elapsedMs - m_lastMetricsMs : elapsedMs;
    const double cpuSeconds = processCpuSeconds();
    const double deltaCpu = cpuSeconds - m_lastCpuSeconds;
    const quint64 deltaFrames = m_frameSwaps - m_lastFrameSwaps;

    m_generatedRateHz = (static_cast<double>(m_generatedFrames) * 1000.0) / elapsedMs;
    m_fps = deltaMs > 0 ? (static_cast<double>(deltaFrames) * 1000.0) / deltaMs : 0.0;
    m_cpuPercent = deltaMs > 0 ? (deltaCpu * 100000.0) / deltaMs : 0.0;
    m_memoryMb = residentMemoryMb();

    m_lastMetricsMs = elapsedMs;
    m_lastCpuSeconds = cpuSeconds;
    m_lastFrameSwaps = m_frameSwaps;
    emit metricsChanged();
}

QVector<_un_anotc_v8_frame> DataChartBenchmarkRunner::makeFrames(int count)
{
    QVector<_un_anotc_v8_frame> frames;
    frames.reserve(count);
    for (int i = 0; i < count; ++i) {
        frames.append(makeEulerFrame(m_sequence++));
    }
    return frames;
}

_un_anotc_v8_frame DataChartBenchmarkRunner::makeEulerFrame(quint32 sequence)
{
    _un_anotc_v8_frame frame {};
    frame.frame.head = ANOTC_V8_HEAD;
    frame.frame.saddr = 0xFE;
    frame.frame.daddr = 0x01;
    frame.frame.fun = ANOTC_FRAME_EULER;
    frame.frame.len = EulerPayloadSize;

    const qint16 roll = static_cast<qint16>(static_cast<qint32>(sequence % 7200U) - 3600);
    const qint16 pitch = static_cast<qint16>(static_cast<qint32>((sequence * 2U) % 3600U) - 1800);
    const qint16 yaw = static_cast<qint16>((sequence * 3U) % 36000U);
    qToLittleEndian<qint16>(roll, frame.frame.data);
    qToLittleEndian<qint16>(pitch, frame.frame.data + 2);
    qToLittleEndian<qint16>(yaw, frame.frame.data + 4);
    frame.frame.data[6] = 1;
    appendChecksum(&frame);
    return frame;
}

double DataChartBenchmarkRunner::processCpuSeconds()
{
    rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }

    const double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
    const double system = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
    return user + system;
}

double DataChartBenchmarkRunner::residentMemoryMb()
{
#ifdef Q_OS_DARWIN
    mach_task_basic_info info {};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return static_cast<double>(info.resident_size) / (1024.0 * 1024.0);
    }
#endif
    rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return static_cast<double>(usage.ru_maxrss) / 1024.0;
    }
    return 0.0;
}
