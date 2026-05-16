#pragma once

#include "ITransport.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QUdpSocket>

class IProtocolSession;

class UdpTransport : public ITransport
{
    Q_OBJECT

public:
    explicit UdpTransport(QObject *parent = nullptr);

    bool open(const ConnectionConfig &config) override;
    void close() override;
    void send(const QByteArray &data) override;
    bool isOpen() const override;
    void setProtocol(IProtocolSession *protocol) override;
    void setRawByteForwardingEnabled(bool enabled) override;

private:
    void ensureSocket();
    void setState(State state);
    void resetPerformanceCounters();
    void maybeEmitPerformanceStats();
    void handleReadyRead();
    void handleSocketError(QAbstractSocket::SocketError error);
    void feedDatagram(const QByteArray &datagram, AnotcParseResult *combinedResult);
    static void appendParseResult(AnotcParseResult *target, const AnotcParseResult &source);

    QUdpSocket *m_socket = nullptr;
    IProtocolSession *m_protocol = nullptr;
    QHostAddress m_remoteAddress;
    quint16 m_remotePort = 0;
    bool m_rawByteForwardingEnabled = false;
    QElapsedTimer m_performanceTimer;
    qint64 m_lastStatsElapsedMs = 0;
    quint64 m_totalBytesReceived = 0;
    quint64 m_totalFeedBytesCalls = 0;
    quint64 m_totalFramesParsed = 0;
    quint64 m_lastStatsBytesReceived = 0;
    quint64 m_lastStatsFeedBytesCalls = 0;
    quint64 m_lastStatsFramesParsed = 0;
    State m_state = State::Closed;
};
