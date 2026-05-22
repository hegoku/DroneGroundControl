#include "DataChartModel.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QTextStream>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace {
constexpr int ChartUpdateIntervalMs = 33;
constexpr int MinimumMaxSamples = 1000;
constexpr int MaximumMaxSamples = 2000000;
constexpr double ZoomInFactor = 0.98;
constexpr double ZoomOutFactor = 1.0 / ZoomInFactor;
}

DataChartModel::DataChartModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_palette({
          QColor("#111111"), QColor("#2f6cff"), QColor("#16c45b"), QColor("#ff1e1e"),
          QColor("#8b5cf6"), QColor("#f59e0b"), QColor("#0891b2"), QColor("#db2777"),
          QColor("#65a30d"), QColor("#475569"), QColor("#dc2626"), QColor("#2563eb")
      })
{
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(ChartUpdateIntervalMs);
    connect(&m_updateTimer, &QTimer::timeout, this, &DataChartModel::flushChartUpdate);

    loadDefinitions(QStringLiteral(":/resources/DataFrameDefination.json"));
    setSampleCapacity(m_maxSamples);
}

int DataChartModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_series.size();
}

QVariant DataChartModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_series.size()) {
        return {};
    }

    const Series &series = m_series.at(index.row());
    switch (role) {
    case NameRole:
        return series.name;
    case ValueRole:
        return series.latestValue;
    case ColorRole:
        return series.color;
    case VisibleRole:
        return series.visible;
    case FunctionRole:
        return series.function;
    case ParameterIndexRole:
        return series.parameterIndex;
    default:
        return {};
    }
}

QHash<int, QByteArray> DataChartModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {ValueRole, "value"},
        {ColorRole, "color"},
        {VisibleRole, "visible"},
        {FunctionRole, "functionId"},
        {ParameterIndexRole, "parameterIndex"}
    };
}

int DataChartModel::count() const
{
    return m_series.size();
}

bool DataChartModel::receiving() const
{
    return m_receiving;
}

void DataChartModel::setReceiving(bool receiving)
{
    if (m_receiving == receiving) {
        return;
    }

    m_receiving = receiving;
    emit receivingChanged();
}

bool DataChartModel::autoScroll() const
{
    return m_autoScroll;
}

void DataChartModel::setAutoScroll(bool autoScroll)
{
    if (m_autoScroll == autoScroll) {
        return;
    }

    m_autoScroll = autoScroll;
    if (m_autoScroll) {
        updateViewRangeAfterSample(m_latestSample);
    }
    emit autoScrollChanged();
    emit chartChanged();
}

bool DataChartModel::cursorEnabled() const
{
    return m_cursorEnabled;
}

void DataChartModel::setCursorEnabled(bool enabled)
{
    if (m_cursorEnabled == enabled) {
        return;
    }

    m_cursorEnabled = enabled;
    emit cursorEnabledChanged();
    emit chartChanged();
}

bool DataChartModel::dragEnabled() const
{
    return m_dragEnabled;
}

void DataChartModel::setDragEnabled(bool enabled)
{
    if (m_dragEnabled == enabled) {
        return;
    }

    m_dragEnabled = enabled;
    emit dragEnabledChanged();
}

bool DataChartModel::cropEnabled() const
{
    return m_cropEnabled;
}

void DataChartModel::setCropEnabled(bool enabled)
{
    if (m_cropEnabled == enabled) {
        return;
    }

    m_cropEnabled = enabled;
    emit cropEnabledChanged();
    emit chartChanged();
}

int DataChartModel::maxSamples() const
{
    return m_maxSamples;
}

void DataChartModel::setMaxSamples(int maxSamples)
{
    const int bounded = std::clamp(maxSamples, MinimumMaxSamples, MaximumMaxSamples);
    if (m_maxSamples == bounded) {
        return;
    }

    m_maxSamples = bounded;
    setSampleCapacity(m_maxSamples);
    for (Series &series : m_series) {
        series.samples.setCapacity(m_maxSamples);
    }
    if (m_visibleSampleSpan > static_cast<double>(m_maxSamples)) {
        m_visibleSampleSpan = static_cast<double>(m_maxSamples);
        emit visibleSampleSpanChanged();
    }
    clearSamples();
    m_viewLower = 0.0;
    m_viewUpper = m_visibleSampleSpan;
    emit maxSamplesChanged();
    emit chartChanged();
}

double DataChartModel::visibleSampleSpan() const
{
    return m_visibleSampleSpan;
}

void DataChartModel::setVisibleSampleSpan(double span)
{
    const double bounded = clampedVisibleSampleSpan(span);
    if (qFuzzyCompare(m_visibleSampleSpan, bounded)) {
        return;
    }

    m_visibleSampleSpan = bounded;
    updateViewRangeAfterSample(m_autoScroll ? m_latestSample : (m_viewLower + m_viewUpper) * 0.5);
    emit visibleSampleSpanChanged();
    emit chartChanged();
}

QString DataChartModel::lastError() const
{
    return m_lastError;
}

void DataChartModel::addSeries(int function, int parameterIndex)
{
    const auto definitionIt = m_definitionByFunction.constFind(function);
    if (definitionIt == m_definitionByFunction.constEnd()) {
        setLastError(QStringLiteral("Unknown data frame function %1.").arg(function));
        return;
    }

    const FrameDefinition &definition = m_definitions.at(*definitionIt);
    if (parameterIndex < 0 || parameterIndex >= definition.parameters.size()) {
        setLastError(QStringLiteral("Unknown parameter %1 for function %2.").arg(parameterIndex).arg(function));
        return;
    }

    const quint64 key = seriesKey(function, parameterIndex);
    if (m_seriesByKey.contains(key)) {
        return;
    }

    const int row = m_series.size();
    beginInsertRows(QModelIndex(), row, row);
    Series series;
    series.key = key;
    series.function = function;
    series.parameterIndex = parameterIndex;
    series.name = uniqueSeriesName(definition.parameters.at(parameterIndex).name);
    series.color = m_palette.at(row % m_palette.size());
    series.samples.setCapacity(m_maxSamples);
    m_series.append(series);
    m_seriesByKey.insert(key, row);
    endInsertRows();

    rebuildSelectedByFunction();
    emit countChanged();
    emit chartChanged();
}

void DataChartModel::ensureBenchmarkSeries()
{
    addSeries(ANOTC_FRAME_EULER, 0);
    addSeries(ANOTC_FRAME_EULER, 1);
    addSeries(ANOTC_FRAME_EULER, 2);
    addSeries(ANOTC_FRAME_EULER, 3);
}

void DataChartModel::removeSeries(int row)
{
    if (row < 0 || row >= m_series.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_series.removeAt(row);
    endRemoveRows();

    m_seriesByKey.clear();
    m_dirtyRows.clear();
    for (int index = 0; index < m_series.size(); ++index) {
        m_seriesByKey.insert(m_series.at(index).key, index);
    }
    rebuildSelectedByFunction();
    emit countChanged();
    emit chartChanged();
}

void DataChartModel::setSeriesVisible(int row, bool visible)
{
    if (row < 0 || row >= m_series.size() || m_series[row].visible == visible) {
        return;
    }

    m_series[row].visible = visible;
    emit dataChanged(index(row, 0), index(row, 0), {VisibleRole});
    emit chartChanged();
}

void DataChartModel::hideAll()
{
    for (int row = 0; row < m_series.size(); ++row) {
        if (!m_series[row].visible) {
            continue;
        }
        m_series[row].visible = false;
        emit dataChanged(index(row, 0), index(row, 0), {VisibleRole});
    }
    emit chartChanged();
}

void DataChartModel::showAll()
{
    for (int row = 0; row < m_series.size(); ++row) {
        if (m_series[row].visible) {
            continue;
        }
        m_series[row].visible = true;
        emit dataChanged(index(row, 0), index(row, 0), {VisibleRole});
    }
    emit chartChanged();
}

void DataChartModel::clearData()
{
    m_dirtyRows.clear();
    for (int row = 0; row < m_series.size(); ++row) {
        Series &series = m_series[row];
        series.samples.clear();
        series.latestValue.clear();
        emit dataChanged(index(row, 0), index(row, 0), {ValueRole});
    }

    clearSamples();
    m_latestSample = 0.0;
    m_viewLower = 0.0;
    m_viewUpper = m_visibleSampleSpan;
    clearManualValueRange();
    emit chartChanged();
}

void DataChartModel::showFullChart()
{
    if (m_sampleCount <= 0) {
        clearManualValueRange();
        m_viewLower = 0.0;
        m_viewUpper = m_visibleSampleSpan;
        emit chartChanged();
        return;
    }

    const double firstSample = static_cast<double>(m_firstSample);
    const double newestSample = static_cast<double>(m_firstSample + static_cast<quint64>(m_sampleCount - 1));
    const double fullSpan = std::max(2.0, newestSample - firstSample);
    const double boundedSpan = clampedVisibleSampleSpan(fullSpan);
    if (!qFuzzyCompare(m_visibleSampleSpan, boundedSpan)) {
        m_visibleSampleSpan = boundedSpan;
        emit visibleSampleSpanChanged();
    }

    const double center = (firstSample + newestSample) * 0.5;
    moveVisibleRange(center - boundedSpan * 0.5);

    clearManualValueRange();
    double yLower = -1.0;
    double yUpper = 1.0;
    valueRange(m_viewLower, m_viewUpper, &yLower, &yUpper);
    setManualValueRange(yLower, yUpper);
}

void DataChartModel::zoomIn()
{
    zoomSampleRange((m_viewLower + m_viewUpper) * 0.5, ZoomInFactor, 0.5);
}

void DataChartModel::zoomOut()
{
    zoomSampleRange((m_viewLower + m_viewUpper) * 0.5, ZoomOutFactor, 0.5);
}

void DataChartModel::panByPixels(double pixelDelta, double pixelWidth)
{
    if (qFuzzyIsNull(pixelDelta) || pixelWidth <= 1.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    const double sampleDelta = -pixelDelta / pixelWidth * m_visibleSampleSpan;
    moveVisibleRange(m_viewLower + sampleDelta);
}

void DataChartModel::panValueRangeByPixels(double pixelDelta, double pixelHeight)
{
    if (qFuzzyIsNull(pixelDelta) || pixelHeight <= 1.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    double currentLower = -1.0;
    double currentUpper = 1.0;
    valueRange(m_viewLower, m_viewUpper, &currentLower, &currentUpper);
    const double span = std::max(1e-9, currentUpper - currentLower);
    const double valueDelta = pixelDelta / pixelHeight * span;
    setManualValueRange(currentLower + valueDelta, currentUpper + valueDelta);
}

void DataChartModel::zoomToSampleRange(double lower, double upper)
{
    if (!std::isfinite(lower) || !std::isfinite(upper)) {
        return;
    }

    if (lower > upper) {
        std::swap(lower, upper);
    }

    const double span = upper - lower;
    if (span < 2.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    const double boundedSpan = clampedVisibleSampleSpan(span);
    if (!qFuzzyCompare(m_visibleSampleSpan, boundedSpan)) {
        m_visibleSampleSpan = boundedSpan;
        emit visibleSampleSpanChanged();
    }
    const double center = (lower + upper) * 0.5;
    moveVisibleRange(center - boundedSpan * 0.5);
}

void DataChartModel::zoomOutFromSampleRange(double lower, double upper)
{
    if (!std::isfinite(lower) || !std::isfinite(upper)) {
        return;
    }

    if (lower > upper) {
        std::swap(lower, upper);
    }

    const double selectedSpan = upper - lower;
    if (selectedSpan < 2.0 || m_visibleSampleSpan <= 0.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    const double center = (lower + upper) * 0.5;
    const double factor = std::clamp(m_visibleSampleSpan / selectedSpan, 1.0, 32.0);
    const double boundedSpan = clampedVisibleSampleSpan(m_visibleSampleSpan * factor);
    if (!qFuzzyCompare(m_visibleSampleSpan, boundedSpan)) {
        m_visibleSampleSpan = boundedSpan;
        emit visibleSampleSpanChanged();
    }
    moveVisibleRange(center - m_visibleSampleSpan * 0.5);
}

void DataChartModel::zoomToChartRange(double xLower, double xUpper, double yLower, double yUpper)
{
    if (!std::isfinite(xLower) || !std::isfinite(xUpper) || !std::isfinite(yLower) || !std::isfinite(yUpper)) {
        return;
    }

    if (xLower > xUpper) {
        std::swap(xLower, xUpper);
    }
    if (yLower > yUpper) {
        std::swap(yLower, yUpper);
    }

    const double selectedXSpan = xUpper - xLower;
    const double selectedYSpan = yUpper - yLower;
    if (selectedXSpan < 2.0 || selectedYSpan <= 1e-9) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    const double xCenter = (xLower + xUpper) * 0.5;
    const double yCenter = (yLower + yUpper) * 0.5;
    const double boundedXSpan = clampedVisibleSampleSpan(selectedXSpan);
    const double boundedYSpan = std::max(selectedYSpan, 1e-9);
    if (!qFuzzyCompare(m_visibleSampleSpan, boundedXSpan)) {
        m_visibleSampleSpan = boundedXSpan;
        emit visibleSampleSpanChanged();
    }
    moveVisibleRange(xCenter - boundedXSpan * 0.5);
    setManualValueRange(yCenter - boundedYSpan * 0.5, yCenter + boundedYSpan * 0.5);
}

void DataChartModel::zoomOutFromChartRange(double xLower, double xUpper, double yLower, double yUpper)
{
    if (!std::isfinite(xLower) || !std::isfinite(xUpper) || !std::isfinite(yLower) || !std::isfinite(yUpper)) {
        return;
    }

    if (xLower > xUpper) {
        std::swap(xLower, xUpper);
    }
    if (yLower > yUpper) {
        std::swap(yLower, yUpper);
    }

    const double selectedXSpan = xUpper - xLower;
    const double selectedYSpan = yUpper - yLower;
    if (selectedXSpan < 2.0 || selectedYSpan <= 1e-9) {
        return;
    }

    double currentYLower = -1.0;
    double currentYUpper = 1.0;
    valueRange(m_viewLower, m_viewUpper, &currentYLower, &currentYUpper);
    const double currentYSpan = std::max(1e-9, currentYUpper - currentYLower);
    const double yFactor = std::clamp(currentYSpan / selectedYSpan, 1.0, 32.0);
    const double newYSpan = currentYSpan * yFactor;
    const double yCenter = (yLower + yUpper) * 0.5;

    zoomOutFromSampleRange(xLower, xUpper);
    setManualValueRange(yCenter - newYSpan * 0.5, yCenter + newYSpan * 0.5);
}

void DataChartModel::zoomValueRange(double anchor, double factor, double anchorRatio)
{
    if (!std::isfinite(anchor) || !std::isfinite(factor) || !std::isfinite(anchorRatio) || factor <= 0.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    double currentLower = -1.0;
    double currentUpper = 1.0;
    valueRange(m_viewLower, m_viewUpper, &currentLower, &currentUpper);
    const double currentSpan = std::max(1e-9, currentUpper - currentLower);
    const double boundedSpan = std::clamp(currentSpan * factor, 1e-9, 1e12);
    const double ratio = std::clamp(anchorRatio, 0.0, 1.0);
    const double lower = anchor - boundedSpan * ratio;
    setManualValueRange(lower, lower + boundedSpan);
}

QString DataChartModel::formatAxisLabel(double value) const
{
    const double magnitude = std::abs(value);
    if (magnitude >= 1000000.0) {
        return QString::number(value, 'E', 0);
    }

    const QLocale locale(QLocale::English, QLocale::UnitedStates);
    const int decimals = qFuzzyIsNull(value - std::round(value))
                             ? 0
                             : (magnitude < 10.0 ? 2 : (magnitude < 100.0 ? 1 : 0));
    QString text = locale.toString(value, 'f', decimals);
    if (decimals > 0) {
        while (text.endsWith(QLatin1Char('0'))) {
            text.chop(1);
        }
        if (text.endsWith(QLatin1Char('.'))) {
            text.chop(1);
        }
    }
    return text;
}

bool DataChartModel::saveCsv(const QUrl &fileUrl)
{
    QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        path = fileUrl.toString();
    }
    if (path.isEmpty()) {
        return false;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path.append(QStringLiteral(".csv"));
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setLastError(QStringLiteral("Failed to open %1 for writing.").arg(path));
        return false;
    }

    QTextStream stream(&file);
    stream << "timestamp";
    for (const Series &series : m_series) {
        stream << "," << csvEscape(series.name);
    }
    stream << "\n";

    if (m_series.isEmpty()) {
        return true;
    }

    if (m_sampleCount == 0) {
        return true;
    }

    QVector<QString> lastValues(m_series.size());
    const quint64 lastSample = m_firstSample + static_cast<quint64>(m_sampleCount - 1);
    for (quint64 sample = m_firstSample; sample <= lastSample; ++sample) {
        const int physical = physicalIndexForSample(sample);
        if (physical < 0) {
            continue;
        }

        for (int seriesIndex = 0; seriesIndex < m_series.size(); ++seriesIndex) {
            double value = 0.0;
            if (m_series.at(seriesIndex).samples.valueAt(physical, &value)) {
                lastValues[seriesIndex] = formatNumber(value);
            }
        }

        const qint64 rowTimestamp = timestampForSample(sample);
        stream << csvEscape(rowTimestamp > 0 ? formatTimestamp(rowTimestamp) : QString::number(sample));
        for (const QString &value : std::as_const(lastValues)) {
            stream << "," << csvEscape(value);
        }
        stream << "\n";
    }

    return true;
}

bool DataChartModel::loadCsv(const QUrl &fileUrl)
{
    QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        path = fileUrl.toString();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLastError(QStringLiteral("Failed to open %1 for reading.").arg(path));
        return false;
    }

    QTextStream stream(&file);
    if (stream.atEnd()) {
        setLastError(QStringLiteral("The selected CSV file is empty."));
        return false;
    }

    bool ok = false;
    const QStringList header = parseCsvLine(stream.readLine(), &ok);
    if (!ok || header.size() < 2) {
        setLastError(QStringLiteral("The selected CSV file is not a valid chart CSV."));
        return false;
    }

    beginResetModel();
    m_series.clear();
    m_seriesByKey.clear();
    m_selectedByFunction.clear();
    m_dirtyRows.clear();
    clearSamples();
    m_viewLower = 0.0;
    m_viewUpper = m_visibleSampleSpan;
    clearManualValueRange();
    for (int column = 1; column < header.size(); ++column) {
        addCsvSeries(header.at(column), m_palette.at((column - 1) % m_palette.size()));
    }
    endResetModel();
    emit countChanged();

    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QStringList row = parseCsvLine(line, &ok);
        if (!ok) {
            setLastError(QStringLiteral("The selected CSV contains an invalid row."));
            return false;
        }

        const quint64 sample = m_nextSample;
        bool timestampOk = false;
        qint64 timestamp = row.isEmpty() ? 0 : row.first().trimmed().toLongLong(&timestampOk);
        if (!timestampOk) {
            const QDateTime parsedTimestamp = QDateTime::fromString(row.isEmpty() ? QString() : row.first().trimmed(),
                                                                    QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
            timestamp = parsedTimestamp.isValid()
                            ? parsedTimestamp.toMSecsSinceEpoch()
                            : QDateTime::currentMSecsSinceEpoch();
        }
        const int physical = appendSample(timestamp);
        for (int column = 1; column < header.size() && column < row.size(); ++column) {
            bool numberOk = false;
            const double value = row.at(column).trimmed().toDouble(&numberOk);
            if (!numberOk || column - 1 >= m_series.size()) {
                continue;
            }

            Series &series = m_series[column - 1];
            series.samples.setValue(physical, value);
            series.latestValue = formatNumber(value);
        }
        m_latestSample = static_cast<double>(sample);
    }

    for (int row = 0; row < m_series.size(); ++row) {
        emit dataChanged(index(row, 0), index(row, 0), {ValueRole});
    }
    showFullChart();
    return true;
}

int DataChartModel::seriesCount() const
{
    return m_series.size();
}

QString DataChartModel::seriesName(int row) const
{
    return row >= 0 && row < m_series.size() ? m_series.at(row).name : QString();
}

QColor DataChartModel::seriesColor(int row) const
{
    return row >= 0 && row < m_series.size() ? m_series.at(row).color : QColor();
}

bool DataChartModel::seriesVisible(int row) const
{
    return row >= 0 && row < m_series.size() && m_series.at(row).visible;
}

QList<QPointF> DataChartModel::decimatedPoints(int row, int pixelWidth) const
{
    QList<QPointF> points;
    if (row < 0 || row >= m_series.size() || pixelWidth <= 0) {
        return points;
    }

    double xLower = 0.0;
    double xUpper = 1.0;
    displayRange(&xLower, &xUpper);
    const Series &series = m_series.at(row);
    if (!series.visible || m_sampleCount == 0) {
        return points;
    }

    const int bucketCount = std::max(1, pixelWidth);
    QVector<double> bucketMin(bucketCount, std::numeric_limits<double>::max());
    QVector<double> bucketMax(bucketCount, std::numeric_limits<double>::lowest());
    QVector<uchar> bucketUsed(bucketCount, 0);

    int firstLogical = 0;
    int lastLogical = -1;
    if (!visibleLogicalRange(xLower, xUpper, &firstLogical, &lastLogical)) {
        return points;
    }

    for (int logical = firstLogical; logical <= lastLogical; ++logical) {
        const quint64 sample = m_firstSample + static_cast<quint64>(logical);
        const double x = static_cast<double>(sample);
        const int physical = physicalIndexForSample(sample);
        double y = 0.0;
        if (physical < 0 || !series.samples.valueAt(physical, &y)) {
            continue;
        }

        const int bucket = std::clamp(static_cast<int>((x - xLower) / (xUpper - xLower) * (bucketCount - 1)), 0, bucketCount - 1);
        bucketMin[bucket] = std::min(bucketMin.at(bucket), y);
        bucketMax[bucket] = std::max(bucketMax.at(bucket), y);
        bucketUsed[bucket] = 1;
    }

    points.reserve(bucketCount * 2);
    for (int bucket = 0; bucket < bucketCount; ++bucket) {
        if (!bucketUsed.at(bucket)) {
            continue;
        }
        const double x = xLower + (xUpper - xLower) * bucket / std::max(1, bucketCount - 1);
        points.append(QPointF(x, bucketMin.at(bucket)));
        if (!qFuzzyCompare(bucketMin.at(bucket), bucketMax.at(bucket))) {
            points.append(QPointF(x, bucketMax.at(bucket)));
        }
    }

    return points;
}

QVariantMap DataChartModel::axisRange() const
{
    double xLower = 0.0;
    double xUpper = 1.0;
    double yLower = -1.0;
    double yUpper = 1.0;
    displayRange(&xLower, &xUpper);
    valueRange(xLower, xUpper, &yLower, &yUpper);
    return {
        {QStringLiteral("xLower"), xLower},
        {QStringLiteral("xUpper"), xUpper},
        {QStringLiteral("yLower"), yLower},
        {QStringLiteral("yUpper"), yUpper}
    };
}

void DataChartModel::processFrames(const QVector<_un_anotc_v8_frame> &frames)
{
    if (!m_receiving || m_selectedByFunction.isEmpty()) {
        return;
    }

    bool changed = false;
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    for (const _un_anotc_v8_frame &frame : frames) {
        const auto selectedIt = m_selectedByFunction.constFind(frame.frame.fun);
        if (selectedIt == m_selectedByFunction.constEnd() || selectedIt->isEmpty()) {
            continue;
        }

        const auto definitionIt = m_definitionByFunction.constFind(frame.frame.fun);
        if (definitionIt == m_definitionByFunction.constEnd()) {
            continue;
        }

        const FrameDefinition &definition = m_definitions.at(*definitionIt);
        const quint64 sample = m_nextSample;
        const int physical = appendSample(timestamp);
        bool frameChanged = false;
        for (int seriesIndex : *selectedIt) {
            if (seriesIndex < 0 || seriesIndex >= m_series.size()) {
                continue;
            }

            Series &series = m_series[seriesIndex];
            if (series.parameterIndex < 0 || series.parameterIndex >= definition.parameters.size()) {
                continue;
            }

            bool ok = false;
            const double value = readNumber(frame.frame.data,
                                            frame.frame.len,
                                            definition.parameters.at(series.parameterIndex),
                                            &ok);
            if (!ok) {
                continue;
            }

            series.samples.setValue(physical, value);
            series.latestValue = formatNumber(value);
            m_dirtyRows.insert(seriesIndex);
            changed = true;
            frameChanged = true;
        }

        if (frameChanged) {
            m_latestSample = static_cast<double>(sample);
            if (m_autoScroll) {
                updateViewRangeAfterSample(m_latestSample);
            }
        }
    }

    if (changed) {
        scheduleChartUpdate();
    }
}

bool DataChartModel::loadDefinitions(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("Failed to read %1.").arg(path));
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        setLastError(QStringLiteral("Invalid data frame definition JSON."));
        return false;
    }

    QVector<FrameDefinition> definitions;
    const QJsonArray frames = document.array();
    definitions.reserve(frames.size());

    for (const QJsonValue &frameValue : frames) {
        const QJsonObject frameObject = frameValue.toObject();
        const int id = frameObject.value(QStringLiteral("id")).toInt(-1);
        if (id < 0 || id > 0xFF) {
            continue;
        }

        FrameDefinition definition;
        definition.function = static_cast<quint8>(id);
        definition.name = frameObject.value(QStringLiteral("name")).toString();

        int offset = 0;
        const QJsonArray parameters = frameObject.value(QStringLiteral("params")).toArray();
        definition.parameters.reserve(parameters.size());
        for (const QJsonValue &parameterValue : parameters) {
            const QJsonObject parameterObject = parameterValue.toObject();
            Parameter parameter;
            parameter.name = parameterObject.value(QStringLiteral("name")).toString();
            parameter.typeName = parameterObject.value(QStringLiteral("type")).toString();
            parameter.type = parseParamType(parameter.typeName);
            parameter.byteSize = byteSize(parameter.type);
            parameter.offset = offset;
            if (parameter.byteSize <= 0) {
                continue;
            }
            offset += parameter.byteSize;
            definition.parameters.append(parameter);
        }

        definitions.append(definition);
    }

    std::sort(definitions.begin(), definitions.end(), [](const FrameDefinition &left, const FrameDefinition &right) {
        return left.function < right.function;
    });

    m_definitions = std::move(definitions);
    m_definitionByFunction.clear();
    for (int index = 0; index < m_definitions.size(); ++index) {
        m_definitionByFunction.insert(m_definitions.at(index).function, index);
    }

    return true;
}

bool DataChartModel::addCsvSeries(const QString &name, const QColor &color)
{
    Series series;
    series.key = static_cast<quint64>(m_series.size() + 1);
    series.function = -1;
    series.parameterIndex = -1;
    series.name = uniqueSeriesName(name.trimmed().isEmpty() ? QStringLiteral("Column %1").arg(m_series.size() + 1) : name);
    series.color = color;
    series.samples.setCapacity(m_maxSamples);
    m_series.append(series);
    return true;
}

void DataChartModel::rebuildSelectedByFunction()
{
    m_selectedByFunction.clear();
    for (int index = 0; index < m_series.size(); ++index) {
        const Series &series = m_series.at(index);
        if (series.function >= 0 && series.parameterIndex >= 0) {
            m_selectedByFunction[series.function].append(index);
        }
    }
}

void DataChartModel::scheduleChartUpdate()
{
    if (!m_updateTimer.isActive()) {
        m_updateTimer.start();
    }
}

void DataChartModel::flushChartUpdate()
{
    if (!m_dirtyRows.isEmpty()) {
        QList<int> rows = m_dirtyRows.values();
        m_dirtyRows.clear();
        std::sort(rows.begin(), rows.end());

        int first = rows.first();
        int last = first;
        for (int i = 1; i < rows.size(); ++i) {
            if (rows.at(i) == last + 1) {
                last = rows.at(i);
                continue;
            }

            emit dataChanged(index(first, 0), index(last, 0), {ValueRole});
            first = rows.at(i);
            last = first;
        }
        emit dataChanged(index(first, 0), index(last, 0), {ValueRole});
    }

    emit chartChanged();
}

void DataChartModel::setSampleCapacity(int capacity)
{
    capacity = std::max(1, capacity);
    m_sampleTimestamps.resize(capacity);
}

void DataChartModel::clearSamples()
{
    m_sampleStart = 0;
    m_sampleCount = 0;
    m_firstSample = 0;
    m_nextSample = 0;
    for (Series &series : m_series) {
        series.samples.clear();
    }
}

int DataChartModel::appendSample(qint64 timestamp)
{
    if (m_sampleTimestamps.isEmpty()) {
        setSampleCapacity(m_maxSamples);
    }

    const quint64 sample = m_nextSample++;
    int physical = 0;
    if (m_sampleCount < m_sampleTimestamps.size()) {
        physical = (m_sampleStart + m_sampleCount) % m_sampleTimestamps.size();
        ++m_sampleCount;
    } else {
        physical = m_sampleStart;
        for (Series &series : m_series) {
            series.samples.clearValue(physical);
        }
        m_sampleStart = (m_sampleStart + 1) % m_sampleTimestamps.size();
        ++m_firstSample;
    }

    m_sampleTimestamps[physical] = timestamp;
    return physical;
}

int DataChartModel::physicalIndexForSample(quint64 sample) const
{
    if (m_sampleTimestamps.isEmpty() || sample < m_firstSample) {
        return -1;
    }

    const quint64 offset = sample - m_firstSample;
    if (offset >= static_cast<quint64>(m_sampleCount)) {
        return -1;
    }

    return (m_sampleStart + static_cast<int>(offset)) % m_sampleTimestamps.size();
}

qint64 DataChartModel::timestampForSample(quint64 sample) const
{
    const int physical = physicalIndexForSample(sample);
    return physical >= 0 ? m_sampleTimestamps.at(physical) : 0;
}

bool DataChartModel::visibleLogicalRange(double xLower, double xUpper, int *firstLogical, int *lastLogical) const
{
    if (m_sampleCount <= 0 || xUpper < static_cast<double>(m_firstSample)) {
        return false;
    }

    const quint64 newestSample = m_firstSample + static_cast<quint64>(m_sampleCount - 1);
    if (xLower > static_cast<double>(newestSample)) {
        return false;
    }

    const double boundedLower = std::max(xLower, static_cast<double>(m_firstSample));
    const double boundedUpper = std::min(xUpper, static_cast<double>(newestSample));
    const int first = std::clamp(static_cast<int>(std::floor(boundedLower - static_cast<double>(m_firstSample))),
                                 0,
                                 m_sampleCount - 1);
    const int last = std::clamp(static_cast<int>(std::ceil(boundedUpper - static_cast<double>(m_firstSample))),
                                0,
                                m_sampleCount - 1);
    if (first > last) {
        return false;
    }

    if (firstLogical) {
        *firstLogical = first;
    }
    if (lastLogical) {
        *lastLogical = last;
    }
    return true;
}

double DataChartModel::clampedVisibleSampleSpan(double span) const
{
    return std::clamp(span, 100.0, static_cast<double>(std::max(MinimumMaxSamples, m_maxSamples)));
}

void DataChartModel::moveVisibleRange(double lower)
{
    const double span = std::max(1.0, m_visibleSampleSpan);
    if (m_sampleCount > 0) {
        const double newestSample = static_cast<double>(m_firstSample + static_cast<quint64>(m_sampleCount - 1));
        if (static_cast<double>(m_sampleCount) >= span) {
            const double minLower = static_cast<double>(m_firstSample);
            const double maxLower = std::max(minLower, newestSample - span);
            lower = std::clamp(lower, minLower, maxLower);
        } else {
            const double minLower = std::max(0.0, newestSample - span);
            const double maxLower = minLower;
            lower = std::clamp(lower, minLower, maxLower);
        }
    } else {
        lower = std::max(0.0, lower);
    }

    const double upper = lower + span;
    if (qFuzzyCompare(m_viewLower, lower) && qFuzzyCompare(m_viewUpper, upper)) {
        return;
    }

    m_viewLower = lower;
    m_viewUpper = upper;
    emit chartChanged();
}

void DataChartModel::setManualValueRange(double lower, double upper)
{
    if (!std::isfinite(lower) || !std::isfinite(upper)) {
        return;
    }

    if (lower > upper) {
        std::swap(lower, upper);
    }
    if (qFuzzyCompare(lower, upper)) {
        lower -= 1.0;
        upper += 1.0;
    }

    if (m_manualValueRange && qFuzzyCompare(m_manualValueLower, lower) && qFuzzyCompare(m_manualValueUpper, upper)) {
        return;
    }

    m_manualValueRange = true;
    m_manualValueLower = lower;
    m_manualValueUpper = upper;
    emit chartChanged();
}

void DataChartModel::clearManualValueRange()
{
    if (!m_manualValueRange) {
        return;
    }

    m_manualValueRange = false;
}

void DataChartModel::zoomSampleRange(double anchor, double factor, double anchorRatio)
{
    if (!std::isfinite(anchor) || !std::isfinite(factor) || !std::isfinite(anchorRatio) || factor <= 0.0) {
        return;
    }

    if (m_autoScroll) {
        setAutoScroll(false);
    }

    const double boundedSpan = clampedVisibleSampleSpan(m_visibleSampleSpan * factor);
    if (!qFuzzyCompare(m_visibleSampleSpan, boundedSpan)) {
        m_visibleSampleSpan = boundedSpan;
        emit visibleSampleSpanChanged();
    }
    const double ratio = std::clamp(anchorRatio, 0.0, 1.0);
    moveVisibleRange(anchor - boundedSpan * ratio);
}

void DataChartModel::setLastError(const QString &message)
{
    if (message.isEmpty() || m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
}

void DataChartModel::updateViewRangeAfterSample(double sample)
{
    if (m_autoScroll) {
        m_viewUpper = std::max(m_visibleSampleSpan, sample);
        m_viewLower = std::max(0.0, m_viewUpper - m_visibleSampleSpan);
        m_viewUpper = m_viewLower + m_visibleSampleSpan;
        return;
    }

    const double center = std::max(0.0, sample);
    m_viewLower = std::max(0.0, center - m_visibleSampleSpan * 0.5);
    m_viewUpper = m_viewLower + m_visibleSampleSpan;
}

bool DataChartModel::displayRange(double *lower, double *upper) const
{
    if (lower) {
        *lower = m_viewLower;
    }
    if (upper) {
        *upper = std::max(m_viewUpper, m_viewLower + 1.0);
    }
    return true;
}

bool DataChartModel::valueRange(double xLower, double xUpper, double *lower, double *upper) const
{
    if (m_manualValueRange) {
        if (lower) {
            *lower = m_manualValueLower;
        }
        if (upper) {
            *upper = std::max(m_manualValueUpper, m_manualValueLower + 1e-9);
        }
        return true;
    }

    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::lowest();
    int firstLogical = 0;
    int lastLogical = -1;
    const bool hasVisibleSamples = visibleLogicalRange(xLower, xUpper, &firstLogical, &lastLogical);

    for (const Series &series : m_series) {
        if (!series.visible || !hasVisibleSamples) {
            continue;
        }
        for (int logical = firstLogical; logical <= lastLogical; ++logical) {
            const quint64 sample = m_firstSample + static_cast<quint64>(logical);
            const int physical = physicalIndexForSample(sample);
            double y = 0.0;
            if (physical < 0 || !series.samples.valueAt(physical, &y)) {
                continue;
            }
            minValue = std::min(minValue, y);
            maxValue = std::max(maxValue, y);
        }
    }

    if (minValue == std::numeric_limits<double>::max()) {
        minValue = -1.0;
        maxValue = 5.0;
    }

    if (qFuzzyCompare(minValue, maxValue)) {
        minValue -= 1.0;
        maxValue += 1.0;
    }

    const double padding = (maxValue - minValue) * 0.08;
    if (lower) {
        *lower = minValue - padding;
    }
    if (upper) {
        *upper = maxValue + padding;
    }
    return true;
}

QString DataChartModel::uniqueSeriesName(const QString &name) const
{
    QString candidate = name;
    if (candidate.trimmed().isEmpty()) {
        candidate = QStringLiteral("Signal");
    }

    int suffix = 2;
    bool found = true;
    while (found) {
        found = false;
        for (const Series &series : m_series) {
            if (series.name == candidate) {
                found = true;
                candidate = QStringLiteral("%1 (%2)").arg(name).arg(suffix++);
                break;
            }
        }
    }
    return candidate;
}

QString DataChartModel::csvEscape(const QString &value) const
{
    QString escaped = value;
    escaped.replace("\"", "\"\"");
    return QStringLiteral("\"%1\"").arg(escaped);
}

QStringList DataChartModel::parseCsvLine(const QString &line, bool *ok) const
{
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QLatin1Char('"')) {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                field.append(QLatin1Char('"'));
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == QLatin1Char(',') && !inQuotes) {
            fields.append(field);
            field.clear();
        } else {
            field.append(ch);
        }
    }

    fields.append(field);
    if (ok) {
        *ok = !inQuotes;
    }
    return fields;
}

QString DataChartModel::formatTimestamp(qint64 timestamp) const
{
    return QDateTime::fromMSecsSinceEpoch(timestamp).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

double DataChartModel::readNumber(const quint8 *payload,
                                  qsizetype payloadLength,
                                  const Parameter &parameter,
                                  bool *ok) const
{
    if (ok) {
        *ok = false;
    }
    if (parameter.byteSize <= 0 || parameter.offset + parameter.byteSize > payloadLength) {
        return 0.0;
    }

    const quint8 *value = payload + parameter.offset;
    double result = 0.0;
    switch (parameter.type) {
    case ParamType::UInt8:
        result = *value;
        break;
    case ParamType::Int8:
        result = static_cast<qint8>(*value);
        break;
    case ParamType::UInt16:
        result = qFromLittleEndian<quint16>(value);
        break;
    case ParamType::Int16:
        result = static_cast<qint16>(qFromLittleEndian<quint16>(value));
        break;
    case ParamType::UInt32:
        result = qFromLittleEndian<quint32>(value);
        break;
    case ParamType::Int32:
        result = static_cast<qint32>(qFromLittleEndian<quint32>(value));
        break;
    case ParamType::Float: {
        const quint32 raw = qFromLittleEndian<quint32>(value);
        float floatValue = 0.0f;
        std::memcpy(&floatValue, &raw, sizeof(floatValue));
        result = floatValue;
        break;
    }
    case ParamType::Unsupported:
        return 0.0;
    }

    if (ok) {
        *ok = true;
    }
    return result;
}

QString DataChartModel::formatNumber(double value) const
{
    return QString::number(value, 'g', 7);
}

DataChartModel::ParamType DataChartModel::parseParamType(const QString &typeName)
{
    const QString normalized = typeName.trimmed().toLower();
    if (normalized == QStringLiteral("uint8")) {
        return ParamType::UInt8;
    }
    if (normalized == QStringLiteral("int8")) {
        return ParamType::Int8;
    }
    if (normalized == QStringLiteral("uint16")) {
        return ParamType::UInt16;
    }
    if (normalized == QStringLiteral("int16")) {
        return ParamType::Int16;
    }
    if (normalized == QStringLiteral("uint32")) {
        return ParamType::UInt32;
    }
    if (normalized == QStringLiteral("int32")) {
        return ParamType::Int32;
    }
    if (normalized == QStringLiteral("float") || normalized == QStringLiteral("float32")) {
        return ParamType::Float;
    }
    return ParamType::Unsupported;
}

int DataChartModel::byteSize(ParamType type)
{
    switch (type) {
    case ParamType::UInt8:
    case ParamType::Int8:
        return 1;
    case ParamType::UInt16:
    case ParamType::Int16:
        return 2;
    case ParamType::UInt32:
    case ParamType::Int32:
    case ParamType::Float:
        return 4;
    case ParamType::Unsupported:
        return 0;
    }
    return 0;
}

quint64 DataChartModel::seriesKey(int function, int parameterIndex)
{
    return (static_cast<quint64>(function) << 32) | static_cast<quint32>(parameterIndex);
}

void DataChartModel::SampleBuffer::setCapacity(int capacity)
{
    capacity = std::max(1, capacity);
    if (capacity == values.size()) {
        return;
    }

    values.resize(capacity);
    validBits.resize((capacity + 63) / 64);
    std::fill(validBits.begin(), validBits.end(), 0);
}

void DataChartModel::SampleBuffer::clear()
{
    std::fill(validBits.begin(), validBits.end(), 0);
}

void DataChartModel::SampleBuffer::clearValue(int physicalIndex)
{
    if (physicalIndex < 0 || physicalIndex >= values.size()) {
        return;
    }

    validBits[physicalIndex / 64] &= ~(quint64(1) << (physicalIndex % 64));
}

void DataChartModel::SampleBuffer::setValue(int physicalIndex, double value)
{
    if (physicalIndex < 0 || physicalIndex >= values.size()) {
        return;
    }

    values[physicalIndex] = static_cast<float>(value);
    validBits[physicalIndex / 64] |= quint64(1) << (physicalIndex % 64);
}

bool DataChartModel::SampleBuffer::valueAt(int physicalIndex, double *value) const
{
    if (physicalIndex < 0 || physicalIndex >= values.size()) {
        return false;
    }

    const bool valid = (validBits.at(physicalIndex / 64) & (quint64(1) << (physicalIndex % 64))) != 0;
    if (!valid) {
        return false;
    }

    if (value) {
        *value = values.at(physicalIndex);
    }
    return true;
}

int DataChartModel::SampleBuffer::capacity() const
{
    return values.size();
}
