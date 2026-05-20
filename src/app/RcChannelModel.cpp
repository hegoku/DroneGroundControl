#include "RcChannelModel.h"

#include <QtGlobal>

namespace {
constexpr int RcChannelCount = 14;
constexpr int RcChannelMin = 1000;
constexpr int RcChannelMax = 2000;
}

RcChannelModel::RcChannelModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_channels = {
        {QStringLiteral("Roll"), 0},
        {QStringLiteral("Throttle"), 2},
        {QStringLiteral("Pitch"), 1},
        {QStringLiteral("Yaw"), 3},
        {QStringLiteral("AUX1"), 4},
        {QStringLiteral("AUX2"), 5},
        {QStringLiteral("AUX3"), 6},
        {QStringLiteral("AUX4"), 7},
        {QStringLiteral("AUX5"), 8},
        {QStringLiteral("AUX6"), 9},
        {QStringLiteral("AUX7"), 10},
        {QStringLiteral("AUX8"), 11},
        {QStringLiteral("AUX9"), 12},
        {QStringLiteral("AUX10"), 13},
    };
}

int RcChannelModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_channels.size();
}

QVariant RcChannelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_channels.size()) {
        return {};
    }

    const Channel &channel = m_channels.at(index.row());
    switch (role) {
    case LabelRole:
        return channel.label;
    case ValueRole:
        return channel.value;
    case ValueTextRole:
        return QString::number(channel.value);
    case NormalizedRole:
        return normalizedValue(channel.value);
    case ActiveRole:
        return channel.active;
    default:
        return {};
    }
}

QHash<int, QByteArray> RcChannelModel::roleNames() const
{
    return {
        {LabelRole, "label"},
        {ValueRole, "value"},
        {ValueTextRole, "valueText"},
        {NormalizedRole, "normalized"},
        {ActiveRole, "active"}
    };
}

int RcChannelModel::count() const
{
    return m_channels.size();
}

bool RcChannelModel::hasSignal() const
{
    return m_hasSignal;
}

quint64 RcChannelModel::frameCount() const
{
    return m_frameCount;
}

void RcChannelModel::processFrames(const QVector<_un_anotc_v8_frame> &frames)
{
    bool receivedRcFrame = false;
    bool changed = false;

    for (const _un_anotc_v8_frame &frame : frames) {
        if (frame.frame.fun != ANOTC_FRAME_RC) {
            continue;
        }

        receivedRcFrame = true;
        ++m_frameCount;

        for (Channel &channel : m_channels) {
            const int offset = channel.payloadIndex * 2;
            if (offset + 1 >= frame.frame.len || channel.payloadIndex < 0 || channel.payloadIndex >= RcChannelCount) {
                if (channel.active) {
                    channel.active = false;
                    changed = true;
                }
                continue;
            }

            const int value = readLeInt16(frame.frame.data + offset);
            if (!channel.active || channel.value != value) {
                channel.value = value;
                channel.active = true;
                changed = true;
            }
        }
    }

    if (!receivedRcFrame) {
        return;
    }

    if (!m_hasSignal) {
        m_hasSignal = true;
        emit signalStateChanged();
    }
    emit frameCountChanged();

    if (changed && !m_channels.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_channels.size() - 1, 0), {ValueRole, ValueTextRole, NormalizedRole, ActiveRole});
    }
}

void RcChannelModel::reset()
{
    bool changed = false;
    for (Channel &channel : m_channels) {
        if (channel.value != 0 || channel.active) {
            channel.value = 0;
            channel.active = false;
            changed = true;
        }
    }

    const bool hadSignal = m_hasSignal;
    m_hasSignal = false;
    m_frameCount = 0;

    if (changed && !m_channels.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_channels.size() - 1, 0), {ValueRole, ValueTextRole, NormalizedRole, ActiveRole});
    }
    if (hadSignal) {
        emit signalStateChanged();
    }
    emit frameCountChanged();
}

int RcChannelModel::readLeInt16(const quint8 *data)
{
    const quint16 value = static_cast<quint16>(data[0])
        | (static_cast<quint16>(data[1]) << 8);
    return static_cast<qint16>(value);
}

double RcChannelModel::normalizedValue(int value)
{
    const double normalized = (static_cast<double>(value) - RcChannelMin)
        / static_cast<double>(RcChannelMax - RcChannelMin);
    return qBound(0.0, normalized, 1.0);
}
