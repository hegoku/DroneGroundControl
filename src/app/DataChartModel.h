#pragma once

#include "../protocol/AnotcFrame.h"

#include <QAbstractListModel>
#include <QColor>
#include <QHash>
#include <QPointF>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class DataChartView;

class DataChartModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool receiving READ receiving WRITE setReceiving NOTIFY receivingChanged)
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    Q_PROPERTY(bool cursorEnabled READ cursorEnabled WRITE setCursorEnabled NOTIFY cursorEnabledChanged)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled NOTIFY dragEnabledChanged)
    Q_PROPERTY(bool cropEnabled READ cropEnabled WRITE setCropEnabled NOTIFY cropEnabledChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int maxSamples READ maxSamples WRITE setMaxSamples NOTIFY maxSamplesChanged)
    Q_PROPERTY(double visibleSampleSpan READ visibleSampleSpan WRITE setVisibleSampleSpan NOTIFY visibleSampleSpanChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        ValueRole,
        ColorRole,
        VisibleRole,
        FunctionRole,
        ParameterIndexRole
    };

    explicit DataChartModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    bool receiving() const;
    void setReceiving(bool receiving);

    bool autoScroll() const;
    void setAutoScroll(bool autoScroll);

    bool cursorEnabled() const;
    void setCursorEnabled(bool enabled);

    bool dragEnabled() const;
    void setDragEnabled(bool enabled);

    bool cropEnabled() const;
    void setCropEnabled(bool enabled);

    int maxSamples() const;
    void setMaxSamples(int maxSamples);

    double visibleSampleSpan() const;
    void setVisibleSampleSpan(double span);

    QString lastError() const;

    Q_INVOKABLE void addSeries(int function, int parameterIndex);
    Q_INVOKABLE void ensureBenchmarkSeries();
    Q_INVOKABLE void removeSeries(int row);
    Q_INVOKABLE void setSeriesVisible(int row, bool visible);
    Q_INVOKABLE void hideAll();
    Q_INVOKABLE void showAll();
    Q_INVOKABLE void clearData();
    Q_INVOKABLE void showFullChart();
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void panByPixels(double pixelDelta, double pixelWidth);
    Q_INVOKABLE void panValueRangeByPixels(double pixelDelta, double pixelHeight);
    Q_INVOKABLE void zoomToSampleRange(double lower, double upper);
    Q_INVOKABLE void zoomOutFromSampleRange(double lower, double upper);
    Q_INVOKABLE void zoomToChartRange(double xLower, double xUpper, double yLower, double yUpper);
    Q_INVOKABLE void zoomOutFromChartRange(double xLower, double xUpper, double yLower, double yUpper);
    Q_INVOKABLE void zoomValueRange(double center, double factor);
    Q_INVOKABLE QString formatAxisLabel(double value) const;
    Q_INVOKABLE bool saveCsv(const QUrl &fileUrl);
    Q_INVOKABLE bool loadCsv(const QUrl &fileUrl);
    Q_INVOKABLE int seriesCount() const;
    Q_INVOKABLE QString seriesName(int row) const;
    Q_INVOKABLE QColor seriesColor(int row) const;
    Q_INVOKABLE bool seriesVisible(int row) const;
    Q_INVOKABLE QList<QPointF> decimatedPoints(int row, int pixelWidth) const;
    Q_INVOKABLE QVariantMap axisRange() const;

public slots:
    void processFrames(const QVector<_un_anotc_v8_frame> &frames);

signals:
    void receivingChanged();
    void autoScrollChanged();
    void cursorEnabledChanged();
    void dragEnabledChanged();
    void cropEnabledChanged();
    void countChanged();
    void maxSamplesChanged();
    void visibleSampleSpanChanged();
    void lastErrorChanged();
    void chartChanged();

private:
    friend class DataChartView;

    enum class ParamType {
        UInt8,
        Int8,
        UInt16,
        Int16,
        UInt32,
        Int32,
        Float,
        Unsupported
    };

    struct Parameter
    {
        QString name;
        QString typeName;
        ParamType type = ParamType::Unsupported;
        int byteSize = 0;
        int offset = 0;
    };

    struct FrameDefinition
    {
        quint8 function = 0;
        QString name;
        QVector<Parameter> parameters;
    };

    struct SampleBuffer
    {
        QVector<float> values;
        QVector<quint64> validBits;

        void setCapacity(int capacity);
        void clear();
        void clearValue(int physicalIndex);
        void setValue(int physicalIndex, double value);
        bool valueAt(int physicalIndex, double *value) const;
        int capacity() const;
    };

    struct Series
    {
        quint64 key = 0;
        int function = 0;
        int parameterIndex = -1;
        QString name;
        QColor color;
        QString latestValue;
        bool visible = true;
        SampleBuffer samples;
    };

    bool loadDefinitions(const QString &path);
    bool addCsvSeries(const QString &name, const QColor &color);
    void rebuildSelectedByFunction();
    void scheduleChartUpdate();
    void flushChartUpdate();
    void setSampleCapacity(int capacity);
    void clearSamples();
    int appendSample(qint64 timestamp);
    int physicalIndexForSample(quint64 sample) const;
    qint64 timestampForSample(quint64 sample) const;
    bool visibleLogicalRange(double xLower, double xUpper, int *firstLogical, int *lastLogical) const;
    double clampedVisibleSampleSpan(double span) const;
    void moveVisibleRange(double lower);
    void setManualValueRange(double lower, double upper);
    void clearManualValueRange();
    void setLastError(const QString &message);
    void updateViewRangeAfterSample(double sample);
    bool displayRange(double *lower, double *upper) const;
    bool valueRange(double xLower, double xUpper, double *lower, double *upper) const;
    QString uniqueSeriesName(const QString &name) const;
    QString csvEscape(const QString &value) const;
    QStringList parseCsvLine(const QString &line, bool *ok = nullptr) const;
    QString formatTimestamp(qint64 timestamp) const;
    double readNumber(const quint8 *payload, qsizetype payloadLength, const Parameter &parameter, bool *ok) const;
    QString formatNumber(double value) const;
    static ParamType parseParamType(const QString &typeName);
    static int byteSize(ParamType type);
    static quint64 seriesKey(int function, int parameterIndex);

    QVector<FrameDefinition> m_definitions;
    QHash<int, int> m_definitionByFunction;
    QVector<Series> m_series;
    QHash<quint64, int> m_seriesByKey;
    QHash<int, QVector<int>> m_selectedByFunction;
    QSet<int> m_dirtyRows;
    QVector<qint64> m_sampleTimestamps;
    QVector<QColor> m_palette;
    QTimer m_updateTimer;
    bool m_receiving = true;
    bool m_autoScroll = true;
    bool m_cursorEnabled = false;
    bool m_dragEnabled = false;
    bool m_cropEnabled = false;
    int m_maxSamples = 1000000;
    double m_visibleSampleSpan = 6000.0;
    double m_viewLower = 0.0;
    double m_viewUpper = 6000.0;
    double m_manualValueLower = -1.0;
    double m_manualValueUpper = 1.0;
    double m_latestSample = 0.0;
    quint64 m_nextSample = 0;
    quint64 m_firstSample = 0;
    int m_sampleStart = 0;
    int m_sampleCount = 0;
    bool m_manualValueRange = false;
    QString m_lastError;
};
