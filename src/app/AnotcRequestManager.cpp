#include "AnotcRequestManager.h"

AnotcRequestManager::AnotcRequestManager(QObject *parent)
    : QObject(parent)
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &AnotcRequestManager::handleTimeout);
}

quint64 AnotcRequestManager::submit(const AnotcRequest &request)
{
    QueueItem item;
    item.requestId = m_nextRequestId++;
    item.request = request;
    enqueue(item);
    return item.requestId;
}

quint64 AnotcRequestManager::submitSequence(const QString &name, const QVector<AnotcRequest> &steps)
{
    const quint64 sequenceId = m_nextSequenceId++;
    if (steps.isEmpty()) {
        emit sequenceFailed(sequenceId, name, QStringLiteral("Sequence has no steps."));
        return sequenceId;
    }

    QueueItem item;
    item.requestId = m_nextRequestId++;
    item.sequenceId = sequenceId;
    item.sequenceName = name;
    item.sequenceSteps = steps;
    item.sequenceStepIndex = 0;
    item.request = steps.first();
    emit sequenceStarted(sequenceId, name);
    enqueue(item);
    return sequenceId;
}

void AnotcRequestManager::cancelAll(const QString &reason)
{
    m_timeoutTimer.stop();

    while (!m_queue.isEmpty()) {
        const QueueItem failed = m_queue.dequeue();
        emit requestFailed(failed.requestId, failed.request.name, reason);
        if (failed.sequenceId != 0) {
            emit sequenceFailed(failed.sequenceId, failed.sequenceName, reason);
        }
    }

    if (m_hasActive) {
        const QueueItem failed = m_active;
        m_hasActive = false;
        emit requestFailed(failed.requestId, failed.request.name, reason);
        if (failed.sequenceId != 0) {
            emit sequenceFailed(failed.sequenceId, failed.sequenceName, reason);
        }
    }
}

AnotcRequest AnotcRequestManager::makeCommandRequest(quint8 cid0,
                                                     quint8 cid1,
                                                     quint8 cid2,
                                                     const QByteArray &values)
{
    QByteArray payload;
    payload.reserve(3 + values.size());
    payload.append(static_cast<char>(cid0));
    payload.append(static_cast<char>(cid1));
    payload.append(static_cast<char>(cid2));
    payload.append(values);

    AnotcRequest request;
    request.name = QStringLiteral("ANOTC command %1.%2.%3")
                       .arg(cid0)
                       .arg(cid1)
                       .arg(cid2);
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CMD_SEND, payload);
    request.responseMatchers.append(AnotcResponseMatcher::ackForFrame(request.encodedFrame));
    return request;
}

AnotcRequest AnotcRequestManager::makeReadDeviceInfoRequest(quint16 type)
{
    QByteArray payload;
    payload.append(static_cast<char>(ANOTC_CONFIG_FRAME_CMD_DEVICE_INFO));
    appendLe16(&payload, type);

    AnotcRequest request;
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_CMD, payload);
    if (type==2) {
        request.name = QStringLiteral("Disconnect to drone");
    } else {
        request.name = QStringLiteral("Read device info");
        request.responseMatchers.append(AnotcResponseMatcher::frameId(ANOTC_FRAME_DEVICE_INFO));
    }
    return request;
}

AnotcRequest AnotcRequestManager::makeReadParameterCountRequest()
{
    QByteArray payload;
    payload.append(static_cast<char>(ANOTC_CONFIG_FRAME_CMD_READ_COUNT));
    appendLe16(&payload, 0);

    AnotcRequest request;
    request.name = QStringLiteral("Read parameter count");
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_CMD, payload);
    request.responseMatchers.append(AnotcResponseMatcher::parameterCommand(ANOTC_CONFIG_FRAME_CMD_READ_COUNT));
    return request;
}

AnotcRequest AnotcRequestManager::makeReadParameterValueRequest(quint16 parameterId)
{
    QByteArray payload;
    payload.append(static_cast<char>(ANOTC_CONFIG_FRAME_CMD_READ_VALUE));
    appendLe16(&payload, parameterId);

    AnotcRequest request;
    request.name = QStringLiteral("Read parameter value %1").arg(parameterId);
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_CMD, payload);
    request.responseMatchers.append(AnotcResponseMatcher::parameterValue(parameterId));
    return request;
}

AnotcRequest AnotcRequestManager::makeReadParameterInfoRequest(quint16 parameterId)
{
    QByteArray payload;
    payload.append(static_cast<char>(ANOTC_CONFIG_FRAME_CMD_READ_INFO));
    appendLe16(&payload, parameterId);

    AnotcRequest request;
    request.name = QStringLiteral("Read parameter info %1").arg(parameterId);
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_CMD, payload);
    request.responseMatchers.append(AnotcResponseMatcher::parameterInfo(parameterId));
    return request;
}

AnotcRequest AnotcRequestManager::makeWriteParameterRequest(quint16 parameterId, const QByteArray &value)
{
    QByteArray payload;
    payload.reserve(2 + value.size());
    appendLe16(&payload, parameterId);
    payload.append(value);

    AnotcRequest request;
    request.name = QStringLiteral("Write parameter %1").arg(parameterId);
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_READ_WRITE, payload);
    request.responseMatchers.append(AnotcResponseMatcher::ackForFrame(request.encodedFrame));
    return request;
}

AnotcRequest AnotcRequestManager::makeReliableParameterCommandRequest(quint8 command, quint16 value)
{
    QByteArray payload;
    payload.append(static_cast<char>(command));
    appendLe16(&payload, value);

    AnotcRequest request;
    request.name = QStringLiteral("Parameter command 0x%1").arg(command, 2, 16, QLatin1Char('0')).toUpper();
    request.encodedFrame = encodeFrame(ANOTC_FRAME_CONFIG_CMD, payload);
    request.responseMatchers.append(AnotcResponseMatcher::ackForFrame(request.encodedFrame));
    return request;
}

void AnotcRequestManager::handleFrames(const AnotcParseResult &result)
{
    if (!m_hasActive) {
        return;
    }

    for (const AnotcResponseMatcher &matcher : m_active.request.responseMatchers) {
        _un_anotc_v8_frame response {};
        if (matcher.matches(result, &response)) {
            completeCurrent(response);
            return;
        }
    }
}

void AnotcRequestManager::handleTimeout()
{
    if (!m_hasActive) {
        return;
    }

    if (m_active.attempts < m_active.request.maxRetries) {
        sendCurrentAttempt();
        return;
    }

    failCurrent(QStringLiteral("Timed out after %1 attempt(s).").arg(m_active.attempts));
}

QByteArray AnotcRequestManager::encodeFrame(quint8 function,
                                            const QByteArray &payload,
                                            quint8 sourceAddress,
                                            quint8 destinationAddress)
{
    if (payload.size() > ANOTC_DATA_MAX_SIZE) {
        return {};
    }

    QByteArray frame;
    frame.reserve(ANOTC_V8_HEAD_SIZE + payload.size() + 2);
    frame.append(static_cast<char>(ANOTC_V8_HEAD));
    frame.append(static_cast<char>(sourceAddress));
    frame.append(static_cast<char>(destinationAddress));
    frame.append(static_cast<char>(function));
    frame.append(static_cast<char>(payload.size() & 0xFF));
    frame.append(static_cast<char>((payload.size() >> 8) & 0xFF));
    frame.append(payload);
    appendChecksum(&frame);
    return frame;
}

void AnotcRequestManager::appendChecksum(QByteArray *frame)
{
    quint8 sum = 0;
    quint8 add = 0;
    for (const char value : *frame) {
        sum = static_cast<quint8>(sum + static_cast<quint8>(value));
        add = static_cast<quint8>(add + sum);
    }

    frame->append(static_cast<char>(sum));
    frame->append(static_cast<char>(add));
}

void AnotcRequestManager::appendLe16(QByteArray *payload, quint16 value)
{
    payload->append(static_cast<char>(value & 0xFF));
    payload->append(static_cast<char>((value >> 8) & 0xFF));
}

void AnotcRequestManager::enqueue(const QueueItem &item)
{
    m_queue.enqueue(item);
    startNext();
}

void AnotcRequestManager::startNext()
{
    if (m_hasActive || m_queue.isEmpty()) {
        return;
    }

    m_active = m_queue.dequeue();
    m_hasActive = true;
    m_active.attempts = 0;
    emit requestStarted(m_active.requestId, m_active.request.name);
    sendCurrentAttempt();
}

void AnotcRequestManager::sendCurrentAttempt()
{
    if (!m_hasActive) {
        return;
    }

    if (m_active.request.encodedFrame.isEmpty()) {
        failCurrent(QStringLiteral("Request frame is empty."));
        return;
    }

    ++m_active.attempts;
    emit log(QStringLiteral("Sending %1 attempt %2/%3")
                 .arg(m_active.request.name)
                 .arg(m_active.attempts)
                 .arg(m_active.request.maxRetries));
    emit sendBytes(m_active.request.encodedFrame);
    m_timeoutTimer.start(m_active.request.timeoutMs);
}

void AnotcRequestManager::completeCurrent(const _un_anotc_v8_frame &response)
{
    m_timeoutTimer.stop();

    QueueItem completed = m_active;
    emit requestSucceeded(completed.requestId, completed.request.name, response);

    if (completed.sequenceId != 0
        && completed.sequenceStepIndex + 1 < completed.sequenceSteps.size()) {
        completed.requestId = m_nextRequestId++;
        completed.sequenceStepIndex += 1;
        completed.request = completed.sequenceSteps.at(completed.sequenceStepIndex);
        completed.attempts = 0;
        m_active = completed;
        emit requestStarted(m_active.requestId, m_active.request.name);
        sendCurrentAttempt();
        return;
    }

    if (completed.sequenceId != 0) {
        emit sequenceSucceeded(completed.sequenceId, completed.sequenceName, response);
    }

    m_hasActive = false;
    m_active = {};
    startNext();
}

void AnotcRequestManager::failCurrent(const QString &reason)
{
    m_timeoutTimer.stop();

    const QueueItem failed = m_active;
    m_hasActive = false;
    m_active = {};

    emit requestFailed(failed.requestId, failed.request.name, reason);
    if (failed.sequenceId != 0) {
        emit sequenceFailed(failed.sequenceId, failed.sequenceName, reason);
    }

    startNext();
}
