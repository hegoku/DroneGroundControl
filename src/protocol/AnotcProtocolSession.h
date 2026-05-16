#pragma once

#include "IProtocolSession.h"

#include <QByteArray>
#include <QtGlobal>
#include <QVector>

class AnotcProtocolSession : public IProtocolSession
{
public:
    AnotcProtocolSession();

    AnotcParseResult feedBytes(const QByteArray &data) override;
    QByteArray encode(const Command &command) override;

    quint64 receiveCount() const override;
    quint64 errorCount() const override;
    quint64 exceededLengthCount() const override;

private:
    enum class DecodeState {
        Head,
        SourceAddress,
        DestinationAddress,
        Function,
        LengthLow,
        LengthHigh,
        Payload,
        SumCheck,
        AddCheck
    };

    void resetParser();
    bool consumeByte(quint8 byte, _un_anotc_v8_frame *frame);
    static void appendFrame(AnotcParseResult *result, const _un_anotc_v8_frame &frame);
    static bool isTelemetryFrame(quint8 function);
    static bool isParameterFrame(quint8 function);
    static bool isCommandFrame(quint8 function);

    QByteArray encodeFrame(quint8 function,
                           const QByteArray &payload,
                           quint8 sourceAddress = DefaultSourceAddress,
                           quint8 destinationAddress = DefaultDestinationAddress) const;
    static void appendChecksum(QByteArray *frame);
    static bool checksumMatches(const _un_anotc_v8_frame &frame, quint8 sumCheck, quint8 addCheck);

    DecodeState m_state = DecodeState::Head;
    _un_anotc_v8_frame m_currentFrame {};
    quint16 m_frameIndex = 0;
    quint16 m_payloadReadCount = 0;
    quint8 m_sumCheck = 0;

    quint64 m_receiveCount = 0;
    quint64 m_errorCount = 0;
    quint64 m_exceededLengthCount = 0;

    static constexpr quint8 DefaultSourceAddress = 0xFE;
    static constexpr quint8 DefaultDestinationAddress = 0x01;
};
