#include "SerialTransport.h"

#include "../protocol/IProtocolSession.h"

#include <QSerialPortInfo>

SerialTransport::SerialTransport(QObject *parent)
    : ITransport(parent)
{
}

bool SerialTransport::open(const ConnectionConfig &config)
{
    ensureSerialPort();
    resetPerformanceCounters();

    if (m_serial->isOpen()) {
        m_serial->close();
    }

    setState(State::Opening);

    bool matchedKnownPort = false;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : ports) {
        if (portInfo.systemLocation() == config.serialPortName
            || portInfo.portName() == config.serialPortName) {
            m_serial->setPort(portInfo);
            matchedKnownPort = true;
            break;
        }
    }

    if (!matchedKnownPort) {
        m_serial->setPortName(config.serialPortName);
    }

    m_serial->setBaudRate(config.baudRate);
    m_serial->setDataBits(parseDataBits(config.dataBits));
    m_serial->setParity(parseParity(config.parity));
    m_serial->setStopBits(parseStopBits(config.stopBit));
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        setState(State::Error);
        emit errorOccurred(m_serial->errorString());
        setState(State::Closed);
        return false;
    }

    setState(State::Open);
    return true;
}

void SerialTransport::close()
{
    if (m_serial && m_serial->isOpen()) {

        while (m_serial->bytesToWrite() > 0) {
            if (!m_serial->waitForBytesWritten(1000)) {
                break;
            }
        }

        setState(State::Closing);
        m_serial->close();
    }

    setState(State::Closed);
}

void SerialTransport::send(const QByteArray &data)
{
    if (!m_serial || !m_serial->isOpen()) {
        emit errorOccurred(QStringLiteral("Serial port is not open."));
        return;
    }

    const qint64 bytesQueued = m_serial->write(data);
    if (bytesQueued < 0) {
        emit errorOccurred(m_serial->errorString());
    }
}

bool SerialTransport::isOpen() const
{
    return m_serial && m_serial->isOpen();
}

void SerialTransport::setProtocol(IProtocolSession *protocol)
{
    m_protocol = protocol;
}

void SerialTransport::setRawByteForwardingEnabled(bool enabled)
{
    m_rawByteForwardingEnabled = enabled;
}

QSerialPort::DataBits SerialTransport::parseDataBits(int dataBits)
{
    switch (dataBits) {
    case 5:
        return QSerialPort::Data5;
    case 6:
        return QSerialPort::Data6;
    case 7:
        return QSerialPort::Data7;
    case 8:
    default:
        return QSerialPort::Data8;
    }
}

QSerialPort::Parity SerialTransport::parseParity(const QString &parity)
{
    if (parity.compare(QStringLiteral("Even"), Qt::CaseInsensitive) == 0) {
        return QSerialPort::EvenParity;
    }
    if (parity.compare(QStringLiteral("Odd"), Qt::CaseInsensitive) == 0) {
        return QSerialPort::OddParity;
    }
    if (parity.compare(QStringLiteral("Mark"), Qt::CaseInsensitive) == 0) {
        return QSerialPort::MarkParity;
    }
    if (parity.compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0) {
        return QSerialPort::SpaceParity;
    }

    return QSerialPort::NoParity;
}

QSerialPort::StopBits SerialTransport::parseStopBits(const QString &stopBit)
{
    if (stopBit == QStringLiteral("1.5")) {
        return QSerialPort::OneAndHalfStop;
    }
    if (stopBit == QStringLiteral("2")) {
        return QSerialPort::TwoStop;
    }

    return QSerialPort::OneStop;
}

void SerialTransport::ensureSerialPort()
{
    if (m_serial) {
        return;
    }

    m_serial = new QSerialPort(this);
    connect(m_serial, &QSerialPort::readyRead, this, &SerialTransport::handleReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialTransport::handleSerialError);
}

void SerialTransport::setState(State state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(m_state);
}

void SerialTransport::resetPerformanceCounters()
{
    m_performanceTimer.restart();
    m_lastStatsElapsedMs = 0;
    m_totalBytesReceived = 0;
    m_totalFeedBytesCalls = 0;
    m_totalFramesParsed = 0;
    m_lastStatsBytesReceived = 0;
    m_lastStatsFeedBytesCalls = 0;
    m_lastStatsFramesParsed = 0;
}

void SerialTransport::maybeEmitPerformanceStats()
{
    if (!m_performanceTimer.isValid()) {
        m_performanceTimer.start();
        return;
    }

    const qint64 elapsedMs = m_performanceTimer.elapsed();
    const qint64 intervalMs = elapsedMs - m_lastStatsElapsedMs;
    if (intervalMs < 1000) {
        return;
    }

    const double intervalSeconds = static_cast<double>(intervalMs) / 1000.0;
    TransportPerformanceStats stats;
    stats.totalBytesReceived = m_totalBytesReceived;
    stats.totalFeedBytesCalls = m_totalFeedBytesCalls;
    stats.totalFramesParsed = m_totalFramesParsed;
    stats.totalChecksumErrors = m_protocol ? m_protocol->errorCount() : 0;
    stats.totalExceededLengthErrors = m_protocol ? m_protocol->exceededLengthCount() : 0;
    stats.bytesPerSecond = static_cast<double>(m_totalBytesReceived - m_lastStatsBytesReceived)
                           / intervalSeconds;
    stats.feedBytesCallsPerSecond = static_cast<double>(m_totalFeedBytesCalls - m_lastStatsFeedBytesCalls)
                                    / intervalSeconds;
    stats.framesParsedPerSecond = static_cast<double>(m_totalFramesParsed - m_lastStatsFramesParsed)
                                  / intervalSeconds;

    m_lastStatsElapsedMs = elapsedMs;
    m_lastStatsBytesReceived = m_totalBytesReceived;
    m_lastStatsFeedBytesCalls = m_totalFeedBytesCalls;
    m_lastStatsFramesParsed = m_totalFramesParsed;

    emit performanceStats(stats);
}

void SerialTransport::handleReadyRead()
{
    if (!m_serial) {
        return;
    }

    const QByteArray data = m_serial->readAll();
    if (!data.isEmpty()) {
        m_totalBytesReceived += static_cast<quint64>(data.size());

        if (m_protocol) {
            ++m_totalFeedBytesCalls;
            const AnotcParseResult result = m_protocol->feedBytes(data);
            m_totalFramesParsed += static_cast<quint64>(result.totalCount());
            if (!result.isEmpty()) {
                emit anotcFramesReceived(result);
            }
        }

        if (m_rawByteForwardingEnabled) {
            emit bytesReceived(data);
        }

        maybeEmitPerformanceStats();
    }
}

void SerialTransport::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    if (m_serial) {
        emit errorOccurred(m_serial->errorString());
    }

    if (error == QSerialPort::ResourceError) {
        close();
    }
}
