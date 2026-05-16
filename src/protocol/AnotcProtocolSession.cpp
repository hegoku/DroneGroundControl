#include "AnotcProtocolSession.h"

AnotcProtocolSession::AnotcProtocolSession()
{
    resetParser();
}

AnotcParseResult AnotcProtocolSession::feedBytes(const QByteArray &data)
{
    AnotcParseResult result;
    result.reserveTelemetry(qMax(1, data.size() / (ANOTC_V8_HEAD_SIZE + 2)));

    for (const char value : data) {
        _un_anotc_v8_frame frame {};
        if (consumeByte(static_cast<quint8>(value), &frame)) {
            appendFrame(&result, frame);
        }
    }

    return result;
}

QByteArray AnotcProtocolSession::encode(const Command &command)
{
    if (command.type == Command::Type::RawBytes) {
        return command.payload;
    }

    if (!command.arguments.contains(QStringLiteral("function"))) {
        return {};
    }

    const quint8 function = static_cast<quint8>(command.arguments.value(QStringLiteral("function")).toUInt());
    const quint8 sourceAddress = static_cast<quint8>(
        command.arguments.value(QStringLiteral("sourceAddress"), DefaultSourceAddress).toUInt());
    const quint8 destinationAddress = static_cast<quint8>(
        command.arguments.value(QStringLiteral("destinationAddress"), DefaultDestinationAddress).toUInt());

    return encodeFrame(function, command.payload, sourceAddress, destinationAddress);
}

quint64 AnotcProtocolSession::receiveCount() const
{
    return m_receiveCount;
}

quint64 AnotcProtocolSession::errorCount() const
{
    return m_errorCount;
}

quint64 AnotcProtocolSession::exceededLengthCount() const
{
    return m_exceededLengthCount;
}

void AnotcProtocolSession::resetParser()
{
    m_state = DecodeState::Head;
    m_currentFrame = {};
    m_frameIndex = 0;
    m_payloadReadCount = 0;
    m_sumCheck = 0;
}

bool AnotcProtocolSession::consumeByte(quint8 byte, _un_anotc_v8_frame *frame)
{
    switch (m_state) {
    case DecodeState::Head:
        if (byte != ANOTC_V8_HEAD) {
            return false;
        }

        resetParser();
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        m_state = DecodeState::SourceAddress;
        break;

    case DecodeState::SourceAddress:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        m_state = DecodeState::DestinationAddress;
        break;

    case DecodeState::DestinationAddress:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        m_state = DecodeState::Function;
        break;

    case DecodeState::Function:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        m_state = DecodeState::LengthLow;
        break;

    case DecodeState::LengthLow:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        m_state = DecodeState::LengthHigh;
        break;

    case DecodeState::LengthHigh:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        if (m_currentFrame.frame.len > ANOTC_DATA_MAX_SIZE) {
            ++m_exceededLengthCount;
            resetParser();
            return false;
        }

        m_state = m_currentFrame.frame.len == 0 ? DecodeState::SumCheck : DecodeState::Payload;
        break;

    case DecodeState::Payload:
        m_currentFrame.rawBytes[m_frameIndex++] = byte;
        ++m_payloadReadCount;
        if (m_payloadReadCount == m_currentFrame.frame.len) {
            m_state = DecodeState::SumCheck;
        }
        break;

    case DecodeState::SumCheck:
        m_sumCheck = byte;
        m_state = DecodeState::AddCheck;
        break;

    case DecodeState::AddCheck:
        if (checksumMatches(m_currentFrame, m_sumCheck, byte)) {
            ++m_receiveCount;
            *frame = m_currentFrame;
            resetParser();
            return true;
        }

        ++m_errorCount;
        resetParser();
        break;
    }

    return false;
}

void AnotcProtocolSession::appendFrame(AnotcParseResult *result, const _un_anotc_v8_frame &frame)
{
    const quint8 function = frame.frame.fun;
    if (function == ANOTC_FRAME_FRAME_CHECK) {
        result->ackFrames.append(frame);
    } else if (function == ANOTC_FRAME_LOG_STRING || function == ANOTC_FRAME_LOG_STRING_NUM) {
        result->logFrames.append(frame);
    } else if (isParameterFrame(function)) {
        result->parameterFrames.append(frame);
    } else if (isCommandFrame(function)) {
        result->commandFrames.append(frame);
    } else if (isTelemetryFrame(function)) {
        result->telemetryFrames.append(frame);
    } else {
        result->unknownFrames.append(frame);
    }
}

bool AnotcProtocolSession::isTelemetryFrame(quint8 function)
{
    return (function >= 0x01 && function <= 0x0F)
        || (function >= 0x20 && function <= 0x21)
        || (function >= 0x30 && function <= 0x35)
        || (function >= 0x40 && function <= 0x41)
        || function == 0x51
        || function == ANOTC_FRAME_CUSTOM_SYSTEM_INFO
        || function == ANOTC_FRAME_CUSTOM_PID;
}

bool AnotcProtocolSession::isParameterFrame(quint8 function)
{
    return function == ANOTC_FRAME_CONFIG_CMD
        || function == ANOTC_FRAME_CONFIG_READ_WRITE
        || function == ANOTC_FRAME_CONFIG_INFO
        || function == ANOTC_FRAME_DEVICE_INFO;
}

bool AnotcProtocolSession::isCommandFrame(quint8 function)
{
    return function == ANOTC_FRAME_CMD_SEND
        || function == 0xC1
        || function == 0xC2;
}

QByteArray AnotcProtocolSession::encodeFrame(quint8 function,
                                             const QByteArray &payload,
                                             quint8 sourceAddress,
                                             quint8 destinationAddress) const
{
    if (payload.size() > ANOTC_DATA_MAX_SIZE) {
        return {};
    }

    QByteArray frameBytes;
    frameBytes.reserve(ANOTC_V8_HEAD_SIZE + payload.size() + 2);
    frameBytes.append(static_cast<char>(ANOTC_V8_HEAD));
    frameBytes.append(static_cast<char>(sourceAddress));
    frameBytes.append(static_cast<char>(destinationAddress));
    frameBytes.append(static_cast<char>(function));
    frameBytes.append(static_cast<char>(payload.size() & 0xFF));
    frameBytes.append(static_cast<char>((payload.size() >> 8) & 0xFF));
    frameBytes.append(payload);
    appendChecksum(&frameBytes);
    return frameBytes;
}

void AnotcProtocolSession::appendChecksum(QByteArray *frame)
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

bool AnotcProtocolSession::checksumMatches(const _un_anotc_v8_frame &frame,
                                           quint8 sumCheck,
                                           quint8 addCheck)
{
    quint8 sum = 0;
    quint8 add = 0;
    const int bytesToCheck = ANOTC_V8_HEAD_SIZE + frame.frame.len;
    for (int i = 0; i < bytesToCheck; ++i) {
        sum = static_cast<quint8>(sum + frame.rawBytes[i]);
        add = static_cast<quint8>(add + sum);
    }

    return sum == sumCheck && add == addCheck;
}
