#include "ConnectionSession.h"

#include "../connection/SerialTransport.h"
#include "../connection/UdpTransport.h"
#include "../protocol/AnotcProtocolSession.h"

#include <QMetaObject>
#include <QProcessEnvironment>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QVariantMap>

ConnectionSession::ConnectionSession(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<ConnectionConfig>("ConnectionConfig");
    qRegisterMetaType<ITransport::State>("ITransport::State");
    qRegisterMetaType<_un_anotc_v8_frame>("_un_anotc_v8_frame");
    qRegisterMetaType<QVector<_un_anotc_v8_frame>>("QVector<_un_anotc_v8_frame>");
    qRegisterMetaType<AnotcParseResult>("AnotcParseResult");
    qRegisterMetaType<TransportPerformanceStats>("TransportPerformanceStats");
    qRegisterMetaType<Command>("Command");

    connect(&m_requestManager, &AnotcRequestManager::sendBytes, this, &ConnectionSession::sendBytes);
    connect(&m_requestManager, &AnotcRequestManager::log, this, &ConnectionSession::log);
    connect(&m_requestManager,
            &AnotcRequestManager::requestStarted,
            this,
            &ConnectionSession::requestStarted);
    connect(&m_requestManager,
            &AnotcRequestManager::requestSucceeded,
            this,
            [this](quint64 requestId, const QString &name, const _un_anotc_v8_frame &response) {
                emit requestSucceeded(requestId, name);
                invokeRequestSuccess(requestId, name, response);
                emit log(QStringLiteral("Request succeeded: %1").arg(name));
            });
    connect(&m_requestManager,
            &AnotcRequestManager::requestFailed,
            this,
            [this](quint64 requestId, const QString &name, const QString &reason) {
                emit requestFailed(requestId, name, reason);
                invokeRequestFailure(requestId, name, reason);
                setLastError(QStringLiteral("%1 failed: %2").arg(name, reason));
            });
    connect(&m_requestManager,
            &AnotcRequestManager::sequenceStarted,
            this,
            &ConnectionSession::sequenceStarted);
    connect(&m_requestManager,
            &AnotcRequestManager::sequenceSucceeded,
            this,
            [this](quint64 sequenceId, const QString &name, const _un_anotc_v8_frame &response) {
                emit sequenceSucceeded(sequenceId, name);
                invokeSequenceSuccess(sequenceId, name, response);
                emit log(QStringLiteral("Sequence succeeded: %1").arg(name));
            });
    connect(&m_requestManager,
            &AnotcRequestManager::sequenceFailed,
            this,
            [this](quint64 sequenceId, const QString &name, const QString &reason) {
                emit sequenceFailed(sequenceId, name, reason);
                invokeSequenceFailure(sequenceId, name, reason);
                setLastError(QStringLiteral("%1 failed: %2").arg(name, reason));
            });

    m_ioThread.setObjectName(QStringLiteral("DroneConnectionIoThread"));
    m_ioThread.start();
}

ConnectionSession::~ConnectionSession()
{
    close();
    m_ioThread.quit();
    m_ioThread.wait();
}

bool ConnectionSession::isOpen() const
{
    return m_isOpen;
}

QString ConnectionSession::state() const
{
    return m_state;
}

QString ConnectionSession::lastError() const
{
    return m_lastError;
}

bool ConnectionSession::rawByteForwardingEnabled() const
{
    return m_rawByteForwardingEnabled;
}

void ConnectionSession::setRawByteForwardingEnabled(bool enabled)
{
    if (m_rawByteForwardingEnabled == enabled) {
        return;
    }

    m_rawByteForwardingEnabled = enabled;

    if (m_transport) {
        QPointer<ITransport> transport = m_transport;
        QMetaObject::invokeMethod(transport, [transport, enabled]() {
            if (transport) {
                transport->setRawByteForwardingEnabled(enabled);
            }
        }, Qt::QueuedConnection);
    }

    emit rawByteForwardingEnabledChanged();
}

QStringList ConnectionSession::availableSerialPorts() const
{
    QStringList ports;
    const auto availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : availablePorts) {
        ports.append(portInfo.systemLocation().isEmpty()
                         ? portInfo.portName()
                         : portInfo.systemLocation());
    }

    const QString extraPorts = QProcessEnvironment::systemEnvironment()
                                   .value(QStringLiteral("DGC_EXTRA_SERIAL_PORTS"));
    const QStringList extras = extraPorts.split(QRegularExpression(QStringLiteral("[,:;]")),
                                                Qt::SkipEmptyParts);
    for (const QString &extra : extras) {
        const QString port = extra.trimmed();
        if (!port.isEmpty() && !ports.contains(port)) {
            ports.append(port);
        }
    }
    return ports;
}

bool ConnectionSession::openSerial(const QString &portName,
                                   int baudRate,
                                   int dataBits,
                                   const QString &parity,
                                   const QString &stopBit)
{
    if (portName.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Serial port is empty."));
        return false;
    }

    close();

    auto *transport = new SerialTransport;
    auto protocol = std::make_unique<AnotcProtocolSession>();
    setTransport(transport);
    setProtocol(std::move(protocol));

    ConnectionConfig config;
    config.transportType = ConnectionConfig::TransportType::Serial;
    config.serialPortName = portName.trimmed();
    config.baudRate = baudRate;
    config.dataBits = dataBits;
    config.parity = parity;
    config.stopBit = stopBit;

    bool opened = false;
    QMetaObject::invokeMethod(m_transport,
                              [&opened, transport, config]() {
                                  opened = transport->open(config);
                              },
                              Qt::BlockingQueuedConnection);

    if (!opened) {
        resetConnectionObjects();
        return false;
    }

    return opened;
}

bool ConnectionSession::openUdp(const QString &host, int remotePort, int localPort)
{
    const QString trimmedHost = host.trimmed();
    if (trimmedHost.isEmpty()) {
        setLastError(QStringLiteral("UDP host is empty."));
        return false;
    }
    if (remotePort <= 0 || remotePort > 65535) {
        setLastError(QStringLiteral("UDP remote port is out of range."));
        return false;
    }
    if (localPort <= 0 || localPort > 65535) {
        setLastError(QStringLiteral("UDP local listen port is out of range."));
        return false;
    }

    close();

    auto *transport = new UdpTransport;
    auto protocol = std::make_unique<AnotcProtocolSession>();
    setTransport(transport);
    setProtocol(std::move(protocol));

    ConnectionConfig config;
    config.transportType = ConnectionConfig::TransportType::Udp;
    config.udpHost = trimmedHost;
    config.udpRemotePort = static_cast<quint16>(remotePort);
    config.udpLocalPort = static_cast<quint16>(localPort);

    bool opened = false;
    QMetaObject::invokeMethod(m_transport,
                              [&opened, transport, config]() {
                                  opened = transport->open(config);
                              },
                              Qt::BlockingQueuedConnection);

    if (!opened) {
        resetConnectionObjects();
        return false;
    }

    return opened;
}

void ConnectionSession::close()
{
    m_requestManager.cancelAll(QStringLiteral("Connection closed."));
    resetConnectionObjects();
}

void ConnectionSession::closeWithDisconnectCommand()
{
    close();

    if (!m_transport) {
        return;
    }

    const QByteArray disconnectFrame = AnotcRequestManager::makeReadDeviceInfoRequest(0x02).encodedFrame;
    QPointer<ITransport> transport = m_transport;
    QMetaObject::invokeMethod(transport, [transport, disconnectFrame]() {
        if (!transport) {
            return;
        }

        if (!disconnectFrame.isEmpty()) {
            transport->send(disconnectFrame);
        }
        transport->close();
        transport->deleteLater();
    }, Qt::BlockingQueuedConnection);

    m_transport.clear();
    m_protocol.reset();
}

void ConnectionSession::setTransport(ITransport *transport)
{
    m_transport = transport;
    m_transport->moveToThread(&m_ioThread);
    QMetaObject::invokeMethod(m_transport, [transport, enabled = m_rawByteForwardingEnabled]() {
        transport->setRawByteForwardingEnabled(enabled);
    }, Qt::BlockingQueuedConnection);

    connect(m_transport, &ITransport::bytesReceived, this, [this](const QByteArray &data) {
        emit bytesReceived(data);
    }, Qt::QueuedConnection);

    connect(m_transport, &ITransport::anotcFramesReceived, this, [this](const AnotcParseResult &result) {
        m_requestManager.handleFrames(result);

        if (!result.telemetryFrames.isEmpty()) {
            emit telemetryFramesReceived(result.telemetryFrames);
        }
        if (!result.parameterFrames.isEmpty()) {
            emit parameterFramesReceived(result.parameterFrames);
        }
        if (!result.ackFrames.isEmpty()) {
            emit ackFramesReceived(result.ackFrames);
        }
        if (!result.commandFrames.isEmpty()) {
            emit commandFramesReceived(result.commandFrames);
        }
        if (!result.logFrames.isEmpty()) {
            emit logFramesReceived(result.logFrames);
        }
        if (!result.unknownFrames.isEmpty()) {
            emit unknownFramesReceived(result.unknownFrames);
        }
    }, Qt::QueuedConnection);

    connect(m_transport, &ITransport::performanceStats, this, [this](const TransportPerformanceStats &stats) {
        emit performanceStats(stats);
        emit log(QStringLiteral("Transport stats: bytes/s=%1 feedBytes/s=%2 parsedFrames/s=%3 totalParsed=%4 checksumErrors=%5 lengthErrors=%6")
                     .arg(stats.bytesPerSecond, 0, 'f', 0)
                     .arg(stats.feedBytesCallsPerSecond, 0, 'f', 0)
                     .arg(stats.framesParsedPerSecond, 0, 'f', 0)
                     .arg(stats.totalFramesParsed)
                     .arg(stats.totalChecksumErrors)
                     .arg(stats.totalExceededLengthErrors));
    }, Qt::QueuedConnection);

    connect(m_transport, &ITransport::stateChanged, this, [this](ITransport::State state) {
        setStateString(transportStateToString(state));
        setOpen(state == ITransport::State::Open);
    }, Qt::QueuedConnection);

    connect(m_transport, &ITransport::errorOccurred, this, [this](const QString &message) {
        setLastError(message);
    }, Qt::QueuedConnection);
}

void ConnectionSession::setProtocol(std::unique_ptr<IProtocolSession> protocol)
{
    m_protocol = std::move(protocol);

    QPointer<ITransport> transport = m_transport;
    IProtocolSession *protocolSession = m_protocol.get();
    QMetaObject::invokeMethod(transport, [transport, protocolSession]() {
        if (transport) {
            transport->setProtocol(protocolSession);
        }
    }, Qt::BlockingQueuedConnection);
}

void ConnectionSession::resetConnectionObjects()
{
    if (m_transport) {
        QPointer<ITransport> transport = m_transport;
        QMetaObject::invokeMethod(transport, [transport]() {
            if (transport) {
                transport->close();
                transport->deleteLater();
            }
        }, Qt::BlockingQueuedConnection);
        m_transport.clear();
    }

    m_protocol.reset();
}

void ConnectionSession::setOpen(bool open)
{
    if (m_isOpen == open) {
        return;
    }

    m_isOpen = open;
    emit isOpenChanged();
}

void ConnectionSession::setStateString(const QString &state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged();
}

void ConnectionSession::setLastError(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }

    m_lastError = message;
    emit errorOccurred(m_lastError);
}

void ConnectionSession::sendBytes(const QByteArray &data)
{
    if (!m_transport || !m_protocol || !m_isOpen) {
        setLastError(QStringLiteral("Connection is not open."));
        return;
    }

    Command command;
    command.type = Command::Type::RawBytes;
    command.payload = data;
    command.name = QStringLiteral("raw");

    QPointer<ITransport> transport = m_transport;
    IProtocolSession *protocol = m_protocol.get();
    QMetaObject::invokeMethod(transport, [transport, protocol, command]() {
        if (!transport || !protocol) {
            return;
        }

        const QByteArray encoded = protocol->encode(command);
        if (!encoded.isEmpty()) {
            transport->send(encoded);
        }
    }, Qt::QueuedConnection);
}

quint64 ConnectionSession::requestDeviceInfo(quint16 type,
                                             const QJSValue &onSuccess,
                                             const QJSValue &onFailure)
{
    return submitRequest(AnotcRequestManager::makeReadDeviceInfoRequest(type), onSuccess, onFailure);
}

quint64 ConnectionSession::requestParameterCount(const QJSValue &onSuccess, const QJSValue &onFailure)
{
    return submitRequest(AnotcRequestManager::makeReadParameterCountRequest(), onSuccess, onFailure);
}

quint64 ConnectionSession::requestParameterValue(int parameterId,
                                                 const QJSValue &onSuccess,
                                                 const QJSValue &onFailure)
{
    if (!isU16Value(parameterId)) {
        setLastError(QStringLiteral("Parameter id is out of range."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("Read parameter value"), lastError());
        return 0;
    }

    return submitRequest(AnotcRequestManager::makeReadParameterValueRequest(static_cast<quint16>(parameterId)),
                         onSuccess,
                         onFailure);
}

quint64 ConnectionSession::requestParameterInfo(int parameterId,
                                                const QJSValue &onSuccess,
                                                const QJSValue &onFailure)
{
    if (!isU16Value(parameterId)) {
        setLastError(QStringLiteral("Parameter id is out of range."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("Read parameter info"), lastError());
        return 0;
    }

    return submitRequest(AnotcRequestManager::makeReadParameterInfoRequest(static_cast<quint16>(parameterId)),
                         onSuccess,
                         onFailure);
}

quint64 ConnectionSession::writeParameterRaw(int parameterId,
                                             const QString &valueHex,
                                             const QJSValue &onSuccess,
                                             const QJSValue &onFailure)
{
    if (!isU16Value(parameterId)) {
        setLastError(QStringLiteral("Parameter id is out of range."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("Write parameter"), lastError());
        return 0;
    }

    bool ok = false;
    const QByteArray value = bytesFromHex(valueHex, &ok);
    if (!ok) {
        setLastError(QStringLiteral("Parameter value hex is invalid."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("Write parameter"), lastError());
        return 0;
    }

    return submitRequest(AnotcRequestManager::makeWriteParameterRequest(static_cast<quint16>(parameterId), value),
                         onSuccess,
                         onFailure);
}

quint64 ConnectionSession::sendReliableParameterCommand(int command,
                                                        int value,
                                                        const QJSValue &onSuccess,
                                                        const QJSValue &onFailure)
{
    if (!isByteValue(command) || !isU16Value(value)) {
        setLastError(QStringLiteral("Parameter command or value is out of range."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("Parameter command"), lastError());
        return 0;
    }

    return submitRequest(AnotcRequestManager::makeReliableParameterCommandRequest(static_cast<quint8>(command),
                                                                                  static_cast<quint16>(value)),
                         onSuccess,
                         onFailure);
}

quint64 ConnectionSession::sendAnotcCommand(int cid0,
                                            int cid1,
                                            int cid2,
                                            const QString &valuesHex,
                                            const QJSValue &onSuccess,
                                            const QJSValue &onFailure)
{
    if (!isByteValue(cid0) || !isByteValue(cid1) || !isByteValue(cid2)) {
        setLastError(QStringLiteral("Command CID byte is out of range."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("ANOTC command"), lastError());
        return 0;
    }

    bool ok = false;
    const QByteArray values = bytesFromHex(valuesHex, &ok);
    if (!ok) {
        setLastError(QStringLiteral("Command values hex is invalid."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("ANOTC command"), lastError());
        return 0;
    }

    return submitRequest(AnotcRequestManager::makeCommandRequest(static_cast<quint8>(cid0),
                                                                 static_cast<quint8>(cid1),
                                                                 static_cast<quint8>(cid2),
                                                                 values),
                         onSuccess,
                         onFailure);
}

quint64 ConnectionSession::sendAnotcCommandSequence(const QVariantList &commands,
                                                    const QJSValue &onSuccess,
                                                    const QJSValue &onFailure)
{
    if (!m_isOpen) {
        setLastError(QStringLiteral("Connection is not open."));
        invokeFailureCallback(onFailure, 0, QStringLiteral("ANOTC command sequence"), lastError());
        return 0;
    }

    QVector<AnotcRequest> steps;
    steps.reserve(commands.size());

    for (const QVariant &commandValue : commands) {
        const QVariantMap command = commandValue.toMap();
        const int cid0 = command.value(QStringLiteral("cid0"), -1).toInt();
        const int cid1 = command.value(QStringLiteral("cid1"), -1).toInt();
        const int cid2 = command.value(QStringLiteral("cid2"), -1).toInt();

        if (!isByteValue(cid0) || !isByteValue(cid1) || !isByteValue(cid2)) {
            setLastError(QStringLiteral("Command sequence contains an invalid CID byte."));
            invokeFailureCallback(onFailure, 0, QStringLiteral("ANOTC command sequence"), lastError());
            return 0;
        }

        bool ok = false;
        const QByteArray values = bytesFromHex(command.value(QStringLiteral("valuesHex")).toString(), &ok);
        if (!ok) {
            setLastError(QStringLiteral("Command sequence contains invalid valuesHex."));
            invokeFailureCallback(onFailure, 0, QStringLiteral("ANOTC command sequence"), lastError());
            return 0;
        }

        AnotcRequest request = AnotcRequestManager::makeCommandRequest(static_cast<quint8>(cid0),
                                                                       static_cast<quint8>(cid1),
                                                                       static_cast<quint8>(cid2),
                                                                       values);
        if (command.contains(QStringLiteral("timeoutMs"))) {
            request.timeoutMs = qMax(1, command.value(QStringLiteral("timeoutMs"), request.timeoutMs).toInt());
        }
        if (command.contains(QStringLiteral("maxRetries"))) {
            request.maxRetries = qMax(1, command.value(QStringLiteral("maxRetries"), request.maxRetries).toInt());
        }

        steps.append(request);
    }

    const QString name = QStringLiteral("ANOTC command sequence");
    const quint64 sequenceId = m_requestManager.submitSequence(name, steps);
    storeSequenceCallbacks(sequenceId, onSuccess, onFailure);
    return sequenceId;
}

quint64 ConnectionSession::submitRequest(const AnotcRequest &request)
{
    if (!m_isOpen) {
        setLastError(QStringLiteral("Connection is not open."));
        return 0;
    }

    return m_requestManager.submit(request);
}

quint64 ConnectionSession::submitRequest(const AnotcRequest &request,
                                         const QJSValue &onSuccess,
                                         const QJSValue &onFailure)
{
    const quint64 requestId = submitRequest(request);
    storeRequestCallbacks(requestId, onSuccess, onFailure);
    if (requestId == 0) {
        invokeFailureCallback(onFailure, 0, request.name, lastError());
    }
    return requestId;
}

void ConnectionSession::storeRequestCallbacks(quint64 requestId,
                                              const QJSValue &onSuccess,
                                              const QJSValue &onFailure)
{
    if (requestId == 0 || (!onSuccess.isCallable() && !onFailure.isCallable())) {
        return;
    }

    m_requestCallbacks.insert(requestId, JsCallbacks { onSuccess, onFailure });
}

void ConnectionSession::storeSequenceCallbacks(quint64 sequenceId,
                                               const QJSValue &onSuccess,
                                               const QJSValue &onFailure)
{
    if (sequenceId == 0 || (!onSuccess.isCallable() && !onFailure.isCallable())) {
        return;
    }

    m_sequenceCallbacks.insert(sequenceId, JsCallbacks { onSuccess, onFailure });
}

void ConnectionSession::invokeRequestSuccess(quint64 requestId,
                                             const QString &name,
                                             const _un_anotc_v8_frame &response)
{
    const JsCallbacks callbacks = m_requestCallbacks.take(requestId);
    invokeSuccessCallback(callbacks.onSuccess, requestId, name, response);
}

void ConnectionSession::invokeRequestFailure(quint64 requestId,
                                             const QString &name,
                                             const QString &reason)
{
    const JsCallbacks callbacks = m_requestCallbacks.take(requestId);
    invokeFailureCallback(callbacks.onFailure, requestId, name, reason);
}

void ConnectionSession::invokeSequenceSuccess(quint64 sequenceId,
                                              const QString &name,
                                              const _un_anotc_v8_frame &response)
{
    const JsCallbacks callbacks = m_sequenceCallbacks.take(sequenceId);
    invokeSuccessCallback(callbacks.onSuccess, sequenceId, name, response);
}

void ConnectionSession::invokeSequenceFailure(quint64 sequenceId,
                                              const QString &name,
                                              const QString &reason)
{
    const JsCallbacks callbacks = m_sequenceCallbacks.take(sequenceId);
    invokeFailureCallback(callbacks.onFailure, sequenceId, name, reason);
}

void ConnectionSession::invokeSuccessCallback(const QJSValue &callback,
                                              quint64 id,
                                              const QString &name,
                                              const _un_anotc_v8_frame &response)
{
    if (!callback.isCallable()) {
        return;
    }

    QJSEngine *engine = qjsEngine(this);
    if (!engine) {
        return;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("id"), QVariant::fromValue(id));
    payload.insert(QStringLiteral("name"), name);
    payload.insert(QStringLiteral("frame"), frameToVariantMap(response));

    QJSValue callable = callback;
    callable.call({ engine->toScriptValue(payload) });
}

void ConnectionSession::invokeFailureCallback(const QJSValue &callback,
                                              quint64 id,
                                              const QString &name,
                                              const QString &reason)
{
    if (!callback.isCallable()) {
        return;
    }

    QJSEngine *engine = qjsEngine(this);
    if (!engine) {
        return;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("id"), QVariant::fromValue(id));
    payload.insert(QStringLiteral("name"), name);
    payload.insert(QStringLiteral("reason"), reason);

    QJSValue callable = callback;
    callable.call({ engine->toScriptValue(payload) });
}

QVariantMap ConnectionSession::frameToVariantMap(const _un_anotc_v8_frame &frame) const
{
    QVariantMap map;
    const quint16 length = frame.frame.len;
    const QByteArray payload(reinterpret_cast<const char *>(frame.frame.data), length);

    map.insert(QStringLiteral("sourceAddress"), frame.frame.saddr);
    map.insert(QStringLiteral("destinationAddress"), frame.frame.daddr);
    map.insert(QStringLiteral("function"), frame.frame.fun);
    map.insert(QStringLiteral("length"), length);
    map.insert(QStringLiteral("payloadHex"), QString::fromLatin1(payload.toHex(' ').toUpper()));

    if (frame.frame.fun == ANOTC_FRAME_FRAME_CHECK && length >= 3) {
        map.insert(QStringLiteral("ackFunction"), frame.frame.data[0]);
        map.insert(QStringLiteral("ackSumCheck"), frame.frame.data[1]);
        map.insert(QStringLiteral("ackAddCheck"), frame.frame.data[2]);
    } else if ((frame.frame.fun == ANOTC_FRAME_CONFIG_READ_WRITE
         || frame.frame.fun == ANOTC_FRAME_CONFIG_INFO)
        && length >= 2) {
        const quint16 parameterId = static_cast<quint16>(frame.frame.data[0])
            | (static_cast<quint16>(frame.frame.data[1]) << 8);
        map.insert(QStringLiteral("parameterId"), parameterId);
        map.insert(QStringLiteral("parameterValueHex"),
                   QString::fromLatin1(payload.mid(2).toHex(' ').toUpper()));
    } else if (frame.frame.fun == ANOTC_FRAME_CONFIG_CMD && length >= 1) {
        map.insert(QStringLiteral("command"), frame.frame.data[0]);
        if (length >= 3) {
            const quint16 value = static_cast<quint16>(frame.frame.data[1])
                | (static_cast<quint16>(frame.frame.data[2]) << 8);
            map.insert(QStringLiteral("value"), value);
        }
    } else if (frame.frame.fun == ANOTC_FRAME_DEVICE_INFO && length >= 1) {
        const quint8 sensorStatus = frame.frame.data[0];
        map.insert(QStringLiteral("sensorStatus"), sensorStatus);
        map.insert(QStringLiteral("imu"), (sensorStatus & 0x01) != 0);
        map.insert(QStringLiteral("mag"), (sensorStatus & 0x02) != 0);
        map.insert(QStringLiteral("baro"), (sensorStatus & 0x04) != 0);

        if (length >= 9) {
            const auto readInt16 = [&frame](int offset) {
                return static_cast<qint16>(static_cast<quint16>(frame.frame.data[offset])
                    | (static_cast<quint16>(frame.frame.data[offset + 1]) << 8));
            };
            map.insert(QStringLiteral("hardwareVersion"), readInt16(1));
            map.insert(QStringLiteral("softwareVersion"), readInt16(3));
            map.insert(QStringLiteral("bootloaderVersion"), readInt16(5));
            map.insert(QStringLiteral("protocolVersion"), readInt16(7));
            map.insert(QStringLiteral("deviceName"), QString::fromUtf8(payload.mid(9)).trimmed());
        }
    }

    return map;
}

QByteArray ConnectionSession::bytesFromHex(const QString &hex, bool *ok)
{
    QString normalized = hex;
    normalized.remove(QRegularExpression(QStringLiteral("\\s+")));

    if (normalized.isEmpty()) {
        if (ok) {
            *ok = true;
        }
        return {};
    }

    static const QRegularExpression validHex(QStringLiteral("^[0-9a-fA-F]+$"));
    const bool valid = normalized.size() % 2 == 0 && validHex.match(normalized).hasMatch();
    if (ok) {
        *ok = valid;
    }
    if (!valid) {
        return {};
    }

    return QByteArray::fromHex(normalized.toLatin1());
}

bool ConnectionSession::isByteValue(int value)
{
    return value >= 0 && value <= 0xFF;
}

bool ConnectionSession::isU16Value(int value)
{
    return value >= 0 && value <= 0xFFFF;
}

QString ConnectionSession::transportStateToString(ITransport::State state)
{
    switch (state) {
    case ITransport::State::Opening:
        return QStringLiteral("opening");
    case ITransport::State::Open:
        return QStringLiteral("open");
    case ITransport::State::Closing:
        return QStringLiteral("closing");
    case ITransport::State::Error:
        return QStringLiteral("error");
    case ITransport::State::Closed:
    default:
        return QStringLiteral("closed");
    }
}
