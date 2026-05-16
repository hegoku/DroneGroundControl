#include "DataChartView.h"

#include <QHoverEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

DataChartView::DataChartView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setAntialiasing(false);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
}

DataChartModel *DataChartView::model() const
{
    return m_model;
}

void DataChartView::setModel(DataChartModel *model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_model = model;
    if (m_model) {
        connect(m_model, &DataChartModel::chartChanged, this, [this]() { update(); });
        connect(m_model, &DataChartModel::rowsInserted, this, [this]() { update(); });
        connect(m_model, &DataChartModel::rowsRemoved, this, [this]() { update(); });
        connect(m_model, &DataChartModel::modelReset, this, [this]() { update(); });
        connect(m_model, &DataChartModel::cursorEnabledChanged, this, [this]() { update(); });
        connect(m_model, &DataChartModel::cropEnabledChanged, this, [this]() { update(); });
    }

    emit modelChanged();
    update();
}

void DataChartView::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(boundingRect(), QColor("#ffffff"));

    const QRectF rect = plotRect();
    painter->setPen(QPen(QColor("#c8cdd4"), 1.0));
    painter->drawRect(rect);

    if (!m_model) {
        return;
    }

    double xLower = 0.0;
    double xUpper = 1.0;
    double yLower = -1.0;
    double yUpper = 1.0;
    m_model->displayRange(&xLower, &xUpper);
    m_model->valueRange(xLower, xUpper, &yLower, &yUpper);

    painter->setClipRect(rect.adjusted(1, 1, -1, -1));
    painter->setPen(QPen(QColor("#d9dde3"), 1.0, Qt::DashLine));
    const int verticalGridCount = 5;
    const int horizontalGridCount = 6;
    for (int i = 1; i <= verticalGridCount; ++i) {
        const qreal x = rect.left() + rect.width() * i / verticalGridCount;
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (int i = 1; i <= horizontalGridCount; ++i) {
        const qreal y = rect.top() + rect.height() * i / horizontalGridCount;
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    const int bucketCount = std::max(1, static_cast<int>(std::ceil(rect.width())));
    QVector<double> bucketMin(bucketCount);
    QVector<double> bucketMax(bucketCount);
    QVector<uchar> bucketUsed(bucketCount);

    for (const DataChartModel::Series &series : std::as_const(m_model->m_series)) {
        if (!series.visible || m_model->m_sampleCount == 0) {
            continue;
        }

        std::fill(bucketMin.begin(), bucketMin.end(), std::numeric_limits<double>::max());
        std::fill(bucketMax.begin(), bucketMax.end(), std::numeric_limits<double>::lowest());
        std::fill(bucketUsed.begin(), bucketUsed.end(), 0);

        int firstLogical = 0;
        int lastLogical = -1;
        if (!m_model->visibleLogicalRange(xLower, xUpper, &firstLogical, &lastLogical)) {
            continue;
        }

        int visibleSamples = 0;
        for (int logical = firstLogical; logical <= lastLogical; ++logical) {
            const quint64 sample = m_model->m_firstSample + static_cast<quint64>(logical);
            const double x = static_cast<double>(sample);
            const int physical = m_model->physicalIndexForSample(sample);
            double y = 0.0;
            if (physical < 0 || !series.samples.valueAt(physical, &y)) {
                continue;
            }

            const int bucket = std::clamp(static_cast<int>((x - xLower) / (xUpper - xLower) * (bucketCount - 1)), 0, bucketCount - 1);
            bucketMin[bucket] = std::min(bucketMin.at(bucket), y);
            bucketMax[bucket] = std::max(bucketMax.at(bucket), y);
            bucketUsed[bucket] = 1;
            ++visibleSamples;
        }

        if (visibleSamples == 0) {
            continue;
        }

        QPen seriesPen(series.color, 1.4);
        seriesPen.setCosmetic(true);
        painter->setPen(seriesPen);

        QPointF previous;
        bool hasPrevious = false;
        for (int bucket = 0; bucket < bucketCount; ++bucket) {
            if (!bucketUsed.at(bucket)) {
                continue;
            }

            const double x = xLower + (xUpper - xLower) * bucket / std::max(1, bucketCount - 1);
            const QPointF minPoint = toPlotPoint(x, bucketMin.at(bucket), xLower, xUpper, yLower, yUpper, rect);
            const QPointF maxPoint = toPlotPoint(x, bucketMax.at(bucket), xLower, xUpper, yLower, yUpper, rect);
            const QPointF midPoint(minPoint.x(), (minPoint.y() + maxPoint.y()) * 0.5);
            if (hasPrevious) {
                painter->drawLine(previous, midPoint);
            }
            painter->drawLine(minPoint, maxPoint);
            previous = midPoint;
            hasPrevious = true;
        }
    }

    painter->setClipping(false);

    painter->setPen(QColor("#1f2933"));
    const QFontMetrics metrics(painter->font());
    for (int i = 0; i <= verticalGridCount; ++i) {
        const double value = xLower + (xUpper - xLower) * i / verticalGridCount;
        const QString label = m_model->formatAxisLabel(value);
        const qreal x = rect.left() + rect.width() * i / verticalGridCount;
        painter->drawText(QPointF(x - metrics.horizontalAdvance(label) * 0.5, rect.bottom() + 24), label);
    }
    for (int i = 0; i <= horizontalGridCount; ++i) {
        const double value = yUpper - (yUpper - yLower) * i / horizontalGridCount;
        const QString label = m_model->formatAxisLabel(value);
        const qreal y = rect.top() + rect.height() * i / horizontalGridCount + metrics.ascent() * 0.35;
        painter->drawText(QPointF(rect.left() - metrics.horizontalAdvance(label) - 10, y), label);
    }

    if (m_model->cursorEnabled() && m_hasCursor) {
        const QPointF cursor = clampToPlot(m_cursorPosition);
        const double sample = xLower + (cursor.x() - rect.left()) / rect.width() * (xUpper - xLower);
        const double value = yUpper - (cursor.y() - rect.top()) / rect.height() * (yUpper - yLower);

        painter->setPen(QPen(QColor("#222222"), 1.0, Qt::DashLine));
        painter->drawLine(QPointF(cursor.x(), rect.top()), QPointF(cursor.x(), rect.bottom()));
        painter->drawLine(QPointF(rect.left(), cursor.y()), QPointF(rect.right(), cursor.y()));
        painter->setBrush(QColor("#ffffff"));
        painter->setPen(QPen(QColor("#555b66"), 1.0));
        painter->drawEllipse(cursor, 4.0, 4.0);

        const QString label = QStringLiteral("X:%1\nY:%2").arg(QString::number(sample, 'f', 0),
                                                               QString::number(value, 'f', 2));
        const QRectF textBounds = QFontMetricsF(painter->font()).boundingRect(QRectF(0, 0, 120, 60),
                                                                               Qt::AlignLeft | Qt::AlignTop,
                                                                               label).adjusted(-8, -6, 8, 6);
        QRectF labelRect(cursor + QPointF(16, -textBounds.height() - 8), textBounds.size());
        if (labelRect.right() > rect.right()) {
            labelRect.moveRight(cursor.x() - 12);
        }
        if (labelRect.top() < rect.top()) {
            labelRect.moveTop(cursor.y() + 12);
        }
        painter->setPen(QPen(QColor("#d4d8de"), 1.0));
        painter->setBrush(QColor("#ffffff"));
        painter->drawRoundedRect(labelRect, 5, 5);
        painter->setPen(QColor("#1f2933"));
        painter->drawText(labelRect.adjusted(8, 6, -8, -6), Qt::AlignLeft | Qt::AlignTop, label);
    }

    if (m_selecting) {
        const QRectF selectionRect(m_selectionStart, m_selectionEnd);
        const QRectF normalized = selectionRect.normalized().intersected(rect);
        if (normalized.width() >= 2.0 && normalized.height() >= 2.0) {
            painter->setPen(QPen(QColor("#2563eb"), 1.0, Qt::DashLine));
            painter->setBrush(QColor(37, 99, 235, 36));
            painter->drawRect(normalized);
        }
    }
}

void DataChartView::hoverMoveEvent(QHoverEvent *event)
{
    updateCursor(event->position());
}

void DataChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_model && m_model->dragEnabled() && m_dragging && (event->buttons() & Qt::RightButton)) {
        const QPointF position = event->position();
        const QRectF rect = plotRect();
        m_model->panByPixels(position.x() - m_lastDragPosition.x(), rect.width());
        m_model->panValueRangeByPixels(position.y() - m_lastDragPosition.y(), rect.height());
        m_lastDragPosition = position;
        event->accept();
        return;
    }

    if (m_model && m_model->cropEnabled() && m_selecting && (event->buttons() & Qt::LeftButton)) {
        m_selectionEnd = clampToPlot(event->position());
        update();
        event->accept();
        return;
    }

    updateCursor(event->position());
}

void DataChartView::mousePressEvent(QMouseEvent *event)
{
    if (m_model && m_model->dragEnabled() && event->button() == Qt::RightButton && plotRect().contains(event->position())) {
        m_dragging = true;
        m_lastDragPosition = event->position();
        event->accept();
        return;
    }

    if (m_model && m_model->cropEnabled() && event->button() == Qt::LeftButton && plotRect().contains(event->position())) {
        m_selecting = true;
        m_selectionStart = clampToPlot(event->position());
        m_selectionEnd = m_selectionStart;
        event->accept();
        update();
        return;
    }

    updateCursor(event->position());
}

void DataChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::RightButton) {
        m_dragging = false;
        event->accept();
        return;
    }

    if (m_model && m_selecting && event->button() == Qt::LeftButton) {
        const QPointF selectionEnd = clampToPlot(event->position());
        const double selectedPixels = std::abs(selectionEnd.x() - m_selectionStart.x());
        const bool zoomIn = selectionEnd.x() > m_selectionStart.x() && selectionEnd.y() > m_selectionStart.y();
        m_selecting = false;
        event->accept();

        if (selectedPixels >= 6.0) {
            const double firstSample = sampleAtPlotX(m_selectionStart.x());
            const double secondSample = sampleAtPlotX(selectionEnd.x());
            const double firstValue = valueAtPlotY(m_selectionStart.y());
            const double secondValue = valueAtPlotY(selectionEnd.y());
            if (zoomIn) {
                m_model->zoomToChartRange(firstSample, secondSample, firstValue, secondValue);
            } else {
                m_model->zoomOutFromChartRange(firstSample, secondSample, firstValue, secondValue);
            }
        }

        update();
        return;
    }

    QQuickPaintedItem::mouseReleaseEvent(event);
}

void DataChartView::wheelEvent(QWheelEvent *event)
{
    if (!m_model) {
        event->ignore();
        return;
    }

    const QPointF cursor = clampToPlot(event->position());
    const double valueCenter = valueAtPlotY(cursor.y());

    if (event->angleDelta().y() > 0) {
        m_model->zoomIn();
        m_model->zoomValueRange(valueCenter, 0.5);
    } else if (event->angleDelta().y() < 0) {
        m_model->zoomOut();
        m_model->zoomValueRange(valueCenter, 2.0);
    }
    event->accept();
}

QRectF DataChartView::plotRect() const
{
    return boundingRect().adjusted(54, 20, -22, -44);
}

QPointF DataChartView::clampToPlot(const QPointF &point) const
{
    const QRectF rect = plotRect();
    return QPointF(std::clamp(point.x(), rect.left(), rect.right()),
                   std::clamp(point.y(), rect.top(), rect.bottom()));
}

QPointF DataChartView::toPlotPoint(double x,
                                   double y,
                                   double xLower,
                                   double xUpper,
                                   double yLower,
                                   double yUpper,
                                   const QRectF &rect) const
{
    const double xRatio = (x - xLower) / std::max(1.0, xUpper - xLower);
    const double yRatio = (y - yLower) / std::max(1e-9, yUpper - yLower);
    return QPointF(rect.left() + xRatio * rect.width(),
                   rect.bottom() - yRatio * rect.height());
}

double DataChartView::sampleAtPlotX(double plotX) const
{
    if (!m_model) {
        return 0.0;
    }

    double xLower = 0.0;
    double xUpper = 1.0;
    m_model->displayRange(&xLower, &xUpper);

    const QRectF rect = plotRect();
    const double ratio = (std::clamp(plotX, rect.left(), rect.right()) - rect.left()) / std::max(1.0, rect.width());
    return xLower + ratio * (xUpper - xLower);
}

double DataChartView::valueAtPlotY(double plotY) const
{
    if (!m_model) {
        return 0.0;
    }

    double xLower = 0.0;
    double xUpper = 1.0;
    double yLower = -1.0;
    double yUpper = 1.0;
    m_model->displayRange(&xLower, &xUpper);
    m_model->valueRange(xLower, xUpper, &yLower, &yUpper);

    const QRectF rect = plotRect();
    const double ratio = (std::clamp(plotY, rect.top(), rect.bottom()) - rect.top()) / std::max(1.0, rect.height());
    return yUpper - ratio * (yUpper - yLower);
}

void DataChartView::updateCursor(const QPointF &point)
{
    m_cursorPosition = clampToPlot(point);
    m_hasCursor = true;
    if (m_model && m_model->cursorEnabled()) {
        update();
    }
}
