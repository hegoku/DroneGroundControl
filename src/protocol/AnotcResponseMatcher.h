#pragma once

#include "AnotcFrame.h"

#include <QByteArray>
#include <QString>
#include <QVector>

class AnotcResponseMatcher
{
public:
    enum class Kind {
        Ack,
        FrameId,
        ParameterValue,
        ParameterInfo,
        ParameterCommand,
        CommandList,
        CommandInfo
    };

    static AnotcResponseMatcher ackForFrame(const QByteArray &encodedFrame);
    static AnotcResponseMatcher frameId(quint8 function);
    static AnotcResponseMatcher parameterValue(quint16 parameterId);
    static AnotcResponseMatcher parameterInfo(quint16 parameterId);
    static AnotcResponseMatcher parameterCommand(quint8 command);
    static AnotcResponseMatcher commandList(quint8 command);
    static AnotcResponseMatcher commandInfo(quint16 commandId);

    bool matches(const _un_anotc_v8_frame &frame) const;
    bool matches(const AnotcParseResult &result, _un_anotc_v8_frame *matchedFrame = nullptr) const;
    QString description() const;

private:
    static quint16 readLe16(const quint8 *data);

    Kind m_kind = Kind::FrameId;
    quint8 m_function = 0;
    quint8 m_command = 0;
    quint16 m_id = 0;
    quint8 m_ackFunction = 0;
    quint8 m_ackSumCheck = 0;
    quint8 m_ackAddCheck = 0;
};
