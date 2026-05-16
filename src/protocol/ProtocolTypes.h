#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVariantMap>

struct Command
{
    enum class Type {
        RawBytes,
        ProtocolFrame
    };

    Type type = Type::RawBytes;
    QByteArray payload;
    QString name;
    QVariantMap arguments;
};

Q_DECLARE_METATYPE(Command)
