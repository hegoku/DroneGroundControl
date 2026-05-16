#pragma once

#include "ConnectionConfig.h"
#include "../protocol/AnotcFrame.h"

#include <QByteArray>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVector>

class IProtocolSession;

struct TransportPerformanceStats
{
    quint64 totalBytesReceived = 0;
    quint64 totalFeedBytesCalls = 0;
    quint64 totalFramesParsed = 0;
    quint64 totalChecksumErrors = 0;
    quint64 totalExceededLengthErrors = 0;
    double bytesPerSecond = 0.0;
    double feedBytesCallsPerSecond = 0.0;
    double framesParsedPerSecond = 0.0;
};

class ITransport : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Closed,
        Opening,
        Open,
        Closing,
        Error
    };
    Q_ENUM(State)

    explicit ITransport(QObject *parent = nullptr) : QObject(parent) {}
    ~ITransport() override = default;

    virtual bool open(const ConnectionConfig &config) = 0;
    virtual void close() = 0;
    virtual void send(const QByteArray &data) = 0;
    virtual bool isOpen() const = 0;
    virtual void setProtocol(IProtocolSession *protocol) { Q_UNUSED(protocol) }
    virtual void setRawByteForwardingEnabled(bool enabled) { Q_UNUSED(enabled) }

signals:
    void bytesReceived(QByteArray data);
    void anotcFramesReceived(AnotcParseResult result);
    void performanceStats(TransportPerformanceStats stats);
    void stateChanged(ITransport::State state);
    void errorOccurred(QString message);
};

Q_DECLARE_METATYPE(ITransport::State)
Q_DECLARE_METATYPE(TransportPerformanceStats)
