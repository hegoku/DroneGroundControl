#pragma once

#include <QtGlobal>
#include <QMetaType>
#include <QString>

struct ConnectionConfig
{
    enum class TransportType {
        Serial,
        Udp
    };

    TransportType transportType = TransportType::Serial;

    QString serialPortName;
    qint32 baudRate = 115200;
    int dataBits = 8;
    QString parity = QStringLiteral("None");
    QString stopBit = QStringLiteral("1");

    QString udpHost;
    quint16 udpRemotePort = 14550;
    quint16 udpLocalPort = 14551;
};

Q_DECLARE_METATYPE(ConnectionConfig)
