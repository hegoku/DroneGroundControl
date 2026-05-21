#pragma once

#include "../protocol/AnotcFrame.h"

#include <QObject>
#include <QVariantList>

class ConnectionSessionBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int frameCmdSend READ frameCmdSend CONSTANT)
    Q_PROPERTY(int commandCalibrateGyro READ commandCalibrateGyro CONSTANT)
    Q_PROPERTY(int commandCalibrateGyroCid0 READ commandCalibrateGyroCid0 CONSTANT)
    Q_PROPERTY(int commandCalibrateGyroCid1 READ commandCalibrateGyroCid1 CONSTANT)
    Q_PROPERTY(int commandCalibrateGyroCid2 READ commandCalibrateGyroCid2 CONSTANT)
    Q_PROPERTY(int commandRebootCid0 READ commandRebootCid0 CONSTANT)
    Q_PROPERTY(int commandRebootCid1 READ commandRebootCid1 CONSTANT)
    Q_PROPERTY(int commandRebootCid2 READ commandRebootCid2 CONSTANT)

public:
    explicit ConnectionSessionBridge(QObject *parent = nullptr);

    int frameCmdSend() const;
    int commandCalibrateGyro() const;
    int commandCalibrateGyroCid0() const;
    int commandCalibrateGyroCid1() const;
    int commandCalibrateGyroCid2() const;
    int commandRebootCid0() const;
    int commandRebootCid1() const;
    int commandRebootCid2() const;

    Q_INVOKABLE QVariantList commandFramePayloads(const QVector<_un_anotc_v8_frame> &frames) const;

private:
    static int cid0(quint32 commandId);
    static int cid1(quint32 commandId);
    static int cid2(quint32 commandId);
};
