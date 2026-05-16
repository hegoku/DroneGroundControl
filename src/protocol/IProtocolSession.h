#pragma once

#include "AnotcFrame.h"
#include "ProtocolTypes.h"

#include <QByteArray>
#include <QVector>

class IProtocolSession
{
public:
    virtual ~IProtocolSession() = default;

    virtual AnotcParseResult feedBytes(const QByteArray &data) = 0;
    virtual QByteArray encode(const Command &command) = 0;
    virtual quint64 receiveCount() const { return 0; }
    virtual quint64 errorCount() const { return 0; }
    virtual quint64 exceededLengthCount() const { return 0; }
};
