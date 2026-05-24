#pragma once

#include "ITransport.h"

#include <QElapsedTimer>
#include <QSerialPort>
#include <QTimer>

class IProtocolSession;

class SerialTransport : public ITransport
{
    Q_OBJECT

public:
    explicit SerialTransport(QObject *parent = nullptr);

    bool open(const ConnectionConfig &config) override;
    void close() override;
    void send(const QByteArray &data) override;
    bool isOpen() const override;
    void setProtocol(IProtocolSession *protocol) override;
    void setRawByteForwardingEnabled(bool enabled) override;

private:
    static QSerialPort::DataBits parseDataBits(int dataBits);
    static QSerialPort::Parity parseParity(const QString &parity);
    static QSerialPort::StopBits parseStopBits(const QString &stopBit);

    void ensureSerialPort();
    void setState(State state);
    void resetPerformanceCounters();
    void maybeEmitPerformanceStats();
    void handleReadyRead();
    void handleSerialError(QSerialPort::SerialPortError error);

    QSerialPort *m_serial = nullptr;
    QTimer *m_statsTimer = nullptr;
    IProtocolSession *m_protocol = nullptr;
    bool m_rawByteForwardingEnabled = false;
    QElapsedTimer m_performanceTimer;
    qint64 m_lastStatsElapsedMs = 0;
    quint64 m_totalBytesReceived = 0;
    quint64 m_totalBytesSent = 0;
    quint64 m_totalFeedBytesCalls = 0;
    quint64 m_totalFramesParsed = 0;
    quint64 m_lastStatsBytesReceived = 0;
    quint64 m_lastStatsBytesSent = 0;
    quint64 m_lastStatsFeedBytesCalls = 0;
    quint64 m_lastStatsFramesParsed = 0;
    State m_state = State::Closed;
};
