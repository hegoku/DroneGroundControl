#pragma once

#include "DataChartModel.h"

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class DataChartGraphsAdapter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DataChartModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int pixelWidth READ pixelWidth WRITE setPixelWidth NOTIFY pixelWidthChanged)

public:
    explicit DataChartGraphsAdapter(QObject *parent = nullptr);

    DataChartModel *model() const;
    void setModel(DataChartModel *model);

    int pixelWidth() const;
    void setPixelWidth(int width);

    Q_INVOKABLE void syncSeries(QObject *series, int row);
    Q_INVOKABLE void syncAxes(QObject *axisX, QObject *axisY);

signals:
    void modelChanged();
    void pixelWidthChanged();

private:
    QPointer<DataChartModel> m_model;
    int m_pixelWidth = 1000;
};
