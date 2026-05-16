#pragma once

#include "DataChartModel.h"

#include <QPointer>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

class DataChartView : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DataChartModel *model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit DataChartView(QQuickItem *parent = nullptr);

    DataChartModel *model() const;
    void setModel(DataChartModel *model);

    void paint(QPainter *painter) override;

signals:
    void modelChanged();

protected:
    void hoverMoveEvent(QHoverEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QRectF plotRect() const;
    QPointF clampToPlot(const QPointF &point) const;
    QPointF toPlotPoint(double x, double y, double xLower, double xUpper, double yLower, double yUpper, const QRectF &rect) const;
    double sampleAtPlotX(double plotX) const;
    double valueAtPlotY(double plotY) const;
    void updateCursor(const QPointF &point);

    QPointer<DataChartModel> m_model;
    QPointF m_cursorPosition;
    QPointF m_lastDragPosition;
    QPointF m_selectionStart;
    QPointF m_selectionEnd;
    bool m_hasCursor = false;
    bool m_dragging = false;
    bool m_selecting = false;
};
