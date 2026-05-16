#include "AnotcResponseMatcher.h"

AnotcResponseMatcher AnotcResponseMatcher::ackForFrame(const QByteArray &encodedFrame)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::Ack;

    if (encodedFrame.size() >= ANOTC_V8_HEAD_SIZE + 2) {
        matcher.m_ackFunction = static_cast<quint8>(encodedFrame.at(3));
        matcher.m_ackSumCheck = static_cast<quint8>(encodedFrame.at(encodedFrame.size() - 2));
        matcher.m_ackAddCheck = static_cast<quint8>(encodedFrame.at(encodedFrame.size() - 1));
    }

    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::frameId(quint8 function)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::FrameId;
    matcher.m_function = function;
    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::parameterValue(quint16 parameterId)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::ParameterValue;
    matcher.m_function = ANOTC_FRAME_CONFIG_READ_WRITE;
    matcher.m_id = parameterId;
    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::parameterInfo(quint16 parameterId)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::ParameterInfo;
    matcher.m_function = ANOTC_FRAME_CONFIG_INFO;
    matcher.m_id = parameterId;
    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::parameterCommand(quint8 command)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::ParameterCommand;
    matcher.m_function = ANOTC_FRAME_CONFIG_CMD;
    matcher.m_command = command;
    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::commandList(quint8 command)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::CommandList;
    matcher.m_function = 0xC1;
    matcher.m_command = command;
    return matcher;
}

AnotcResponseMatcher AnotcResponseMatcher::commandInfo(quint16 commandId)
{
    AnotcResponseMatcher matcher;
    matcher.m_kind = Kind::CommandInfo;
    matcher.m_function = 0xC2;
    matcher.m_id = commandId;
    return matcher;
}

bool AnotcResponseMatcher::matches(const _un_anotc_v8_frame &frame) const
{
    const quint8 function = frame.frame.fun;
    const quint16 length = frame.frame.len;

    switch (m_kind) {
    case Kind::Ack:
        return function == ANOTC_FRAME_FRAME_CHECK
            && length >= 3
            && frame.frame.data[0] == m_ackFunction
            && frame.frame.data[1] == m_ackSumCheck
            && frame.frame.data[2] == m_ackAddCheck;

    case Kind::FrameId:
        return function == m_function;

    case Kind::ParameterValue:
    case Kind::ParameterInfo:
        return function == m_function
            && length >= 2
            && readLe16(frame.frame.data) == m_id;

    case Kind::ParameterCommand:
        return function == ANOTC_FRAME_CONFIG_CMD
            && length >= 1
            && frame.frame.data[0] == m_command;

    case Kind::CommandList:
        return function == 0xC1
            && length >= 1
            && frame.frame.data[0] == m_command;

    case Kind::CommandInfo:
        return function == 0xC2
            && length >= 2
            && readLe16(frame.frame.data) == m_id;
    }

    return false;
}

bool AnotcResponseMatcher::matches(const AnotcParseResult &result, _un_anotc_v8_frame *matchedFrame) const
{
    const QVector<const QVector<_un_anotc_v8_frame> *> frameGroups = {
        &result.ackFrames,
        &result.parameterFrames,
        &result.commandFrames,
        &result.logFrames,
        &result.unknownFrames
    };

    for (const QVector<_un_anotc_v8_frame> *frames : frameGroups) {
        for (const _un_anotc_v8_frame &frame : *frames) {
            if (matches(frame)) {
                if (matchedFrame) {
                    *matchedFrame = frame;
                }
                return true;
            }
        }
    }

    return false;
}

QString AnotcResponseMatcher::description() const
{
    switch (m_kind) {
    case Kind::Ack:
        return QStringLiteral("0x00 ack for frame 0x%1")
            .arg(m_ackFunction, 2, 16, QLatin1Char('0')).toUpper();
    case Kind::FrameId:
        return QStringLiteral("frame 0x%1").arg(m_function, 2, 16, QLatin1Char('0')).toUpper();
    case Kind::ParameterValue:
        return QStringLiteral("parameter value %1").arg(m_id);
    case Kind::ParameterInfo:
        return QStringLiteral("parameter info %1").arg(m_id);
    case Kind::ParameterCommand:
        return QStringLiteral("parameter command %1").arg(m_command);
    case Kind::CommandList:
        return QStringLiteral("command list response %1").arg(m_command);
    case Kind::CommandInfo:
        return QStringLiteral("command info %1").arg(m_id);
    }

    return QStringLiteral("unknown response");
}

quint16 AnotcResponseMatcher::readLe16(const quint8 *data)
{
    return static_cast<quint16>(data[0])
        | (static_cast<quint16>(data[1]) << 8);
}
