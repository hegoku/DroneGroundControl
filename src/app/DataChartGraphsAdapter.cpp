#include "DataChartGraphsAdapter.h"

#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

DataChartGraphsAdapter::DataChartGraphsAdapter(QObject *parent)
    : QObject(parent)
{
}

DataChartModel *DataChartGraphsAdapter::model() const
{
    return m_model;
}

void DataChartGraphsAdapter::setModel(DataChartModel *model)
{
    if (m_model == model) {
        return;
    }

    m_model = model;
    emit modelChanged();
}

int DataChartGraphsAdapter::pixelWidth() const
{
    return m_pixelWidth;
}

void DataChartGraphsAdapter::setPixelWidth(int width)
{
    const int bounded = qMax(1, width);
    if (m_pixelWidth == bounded) {
        return;
    }

    m_pixelWidth = bounded;
    emit pixelWidthChanged();
}

void DataChartGraphsAdapter::syncSeries(QObject *seriesObject, int row)
{
    if (!m_model || !seriesObject) {
        return;
    }

    auto *series = qobject_cast<QLineSeries *>(seriesObject);
    if (!series) {
        return;
    }

    series->setName(m_model->seriesName(row));
    series->setColor(m_model->seriesColor(row));
    series->setVisible(m_model->seriesVisible(row));
    series->replace(m_model->decimatedPoints(row, m_pixelWidth));
}

void DataChartGraphsAdapter::syncAxes(QObject *axisXObject, QObject *axisYObject)
{
    if (!m_model) {
        return;
    }

    auto *axisX = qobject_cast<QValueAxis *>(axisXObject);
    auto *axisY = qobject_cast<QValueAxis *>(axisYObject);
    if (!axisX || !axisY) {
        return;
    }

    const QVariantMap range = m_model->axisRange();
    axisX->setMin(range.value(QStringLiteral("xLower")).toDouble());
    axisX->setMax(range.value(QStringLiteral("xUpper")).toDouble());
    axisY->setMin(range.value(QStringLiteral("yLower")).toDouble());
    axisY->setMax(range.value(QStringLiteral("yUpper")).toDouble());
}
