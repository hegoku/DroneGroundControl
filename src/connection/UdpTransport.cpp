#include "UdpTransport.h"

#include "../protocol/IProtocolSession.h"

#include <QAbstractSocket>

UdpTransport::UdpTransport(QObject *parent)
    : ITransport(parent)
{
}

bool UdpTransport::open(const ConnectionConfig &config)
{
    ensureSocket();
    resetPerformanceCounters();

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->close();
    }

    setState(State::Opening);

    QHostAddress remoteAddress;
    if (!remoteAddress.setAddress(config.udpHost.trimmed())) {
        setState(State::Error);
        emit errorOccurred(QStringLiteral("Invalid UDP host address: %1").arg(config.udpHost));
        setState(State::Closed);
        return false;
    }

    const bool bound = m_socket->bind(QHostAddress::AnyIPv4,
                                      config.udpLocalPort,
                                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!bound) {
        setState(State::Error);
        emit errorOccurred(m_socket->errorString());
        setState(State::Closed);
        return false;
    }

    m_remoteAddress = remoteAddress;
    m_remotePort = config.udpRemotePort;
    setState(State::Open);
    return true;
}

void UdpTransport::close()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        setState(State::Closing);
        m_socket->close();
    }

    setState(State::Closed);
}

void UdpTransport::send(const QByteArray &data)
{
    if (!m_socket || m_socket->state() == QAbstractSocket::UnconnectedState) {
        emit errorOccurred(QStringLiteral("UDP socket is not open."));
        return;
    }

    const qint64 bytesWritten = m_socket->writeDatagram(data, m_remoteAddress, m_remotePort);
    if (bytesWritten < 0) {
        emit errorOccurred(m_socket->errorString());
    }
}

bool UdpTransport::isOpen() const
{
    return m_socket && m_socket->state() != QAbstractSocket::UnconnectedState;
}

void UdpTransport::setProtocol(IProtocolSession *protocol)
{
    m_protocol = protocol;
}

void UdpTransport::setRawByteForwardingEnabled(bool enabled)
{
    m_rawByteForwardingEnabled = enabled;
}

void UdpTransport::ensureSocket()
{
    if (m_socket) {
        return;
    }

    m_socket = new QUdpSocket(this);
    m_socket->setReadBufferSize(1024 * 1024);
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpTransport::handleReadyRead);
    connect(m_socket, &QUdpSocket::errorOccurred, this, &UdpTransport::handleSocketError);
}

void UdpTransport::setState(State state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(m_state);
}

void UdpTransport::resetPerformanceCounters()
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

void UdpTransport::maybeEmitPerformanceStats()
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

void UdpTransport::handleReadyRead()
{
    if (!m_socket) {
        return;
    }

    QByteArray forwardedBytes;
    AnotcParseResult combinedResult;

    while (m_socket->hasPendingDatagrams()) {
        const qint64 datagramSize = m_socket->pendingDatagramSize();
        if (datagramSize <= 0) {
            break;
        }

        QByteArray datagram;
        datagram.resize(static_cast<qsizetype>(datagramSize));
        const qint64 bytesRead = m_socket->readDatagram(datagram.data(), datagram.size());
        if (bytesRead < 0) {
            emit errorOccurred(m_socket->errorString());
            continue;
        }

        datagram.truncate(static_cast<qsizetype>(bytesRead));
        m_totalBytesReceived += static_cast<quint64>(datagram.size());

        feedDatagram(datagram, &combinedResult);

        if (m_rawByteForwardingEnabled) {
            forwardedBytes.append(datagram);
        }
    }

    if (!combinedResult.isEmpty()) {
        emit anotcFramesReceived(combinedResult);
    }

    if (!forwardedBytes.isEmpty()) {
        emit bytesReceived(forwardedBytes);
    }

    maybeEmitPerformanceStats();
}

void UdpTransport::handleSocketError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::UnknownSocketError) {
        return;
    }

    if (m_socket) {
        emit errorOccurred(m_socket->errorString());
    }
}

void UdpTransport::feedDatagram(const QByteArray &datagram, AnotcParseResult *combinedResult)
{
    if (!m_protocol || datagram.isEmpty()) {
        return;
    }

    ++m_totalFeedBytesCalls;
    const AnotcParseResult result = m_protocol->feedBytes(datagram);
    m_totalFramesParsed += static_cast<quint64>(result.totalCount());
    appendParseResult(combinedResult, result);
}

void UdpTransport::appendParseResult(AnotcParseResult *target, const AnotcParseResult &source)
{
    if (source.isEmpty()) {
        return;
    }

    target->telemetryFrames += source.telemetryFrames;
    target->parameterFrames += source.parameterFrames;
    target->ackFrames += source.ackFrames;
    target->commandFrames += source.commandFrames;
    target->logFrames += source.logFrames;
    target->unknownFrames += source.unknownFrames;
}
