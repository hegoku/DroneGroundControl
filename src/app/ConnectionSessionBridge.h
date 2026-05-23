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
    Q_PROPERTY(int commandCalibrateAccel READ commandCalibrateAccel CONSTANT)
    Q_PROPERTY(int commandCalibrateAccelCid0 READ commandCalibrateAccelCid0 CONSTANT)
    Q_PROPERTY(int commandCalibrateAccelCid1 READ commandCalibrateAccelCid1 CONSTANT)
    Q_PROPERTY(int commandCalibrateAccelCid2 READ commandCalibrateAccelCid2 CONSTANT)
    Q_PROPERTY(int accelDirectionStart READ accelDirectionStart CONSTANT)
    Q_PROPERTY(int accelDirectionUp READ accelDirectionUp CONSTANT)
    Q_PROPERTY(int accelDirectionDown READ accelDirectionDown CONSTANT)
    Q_PROPERTY(int accelDirectionForward READ accelDirectionForward CONSTANT)
    Q_PROPERTY(int accelDirectionBackward READ accelDirectionBackward CONSTANT)
    Q_PROPERTY(int accelDirectionLeft READ accelDirectionLeft CONSTANT)
    Q_PROPERTY(int accelDirectionRight READ accelDirectionRight CONSTANT)
    Q_PROPERTY(int commandRebootCid0 READ commandRebootCid0 CONSTANT)
    Q_PROPERTY(int commandRebootCid1 READ commandRebootCid1 CONSTANT)
    Q_PROPERTY(int commandRebootCid2 READ commandRebootCid2 CONSTANT)
    Q_PROPERTY(int commandToggleMotorTestCid0 READ commandToggleMotorTestCid0 CONSTANT)
    Q_PROPERTY(int commandToggleMotorTestCid1 READ commandToggleMotorTestCid1 CONSTANT)
    Q_PROPERTY(int commandToggleMotorTestCid2 READ commandToggleMotorTestCid2 CONSTANT)
    Q_PROPERTY(int commandMotorTestThrottleCid0 READ commandMotorTestThrottleCid0 CONSTANT)
    Q_PROPERTY(int commandMotorTestThrottleCid1 READ commandMotorTestThrottleCid1 CONSTANT)
    Q_PROPERTY(int commandMotorTestThrottleCid2 READ commandMotorTestThrottleCid2 CONSTANT)
    Q_PROPERTY(int framePwm READ framePwm CONSTANT)
    Q_PROPERTY(int frameCustomSystemInfo READ frameCustomSystemInfo CONSTANT)
    Q_PROPERTY(int frameCustomPid READ frameCustomPid CONSTANT)
    Q_PROPERTY(int flightStatusMotorTest READ flightStatusMotorTest CONSTANT)

public:
    explicit ConnectionSessionBridge(QObject *parent = nullptr);

    int frameCmdSend() const;
    int commandCalibrateGyro() const;
    int commandCalibrateGyroCid0() const;
    int commandCalibrateGyroCid1() const;
    int commandCalibrateGyroCid2() const;
    int commandCalibrateAccel() const;
    int commandCalibrateAccelCid0() const;
    int commandCalibrateAccelCid1() const;
    int commandCalibrateAccelCid2() const;
    int accelDirectionStart() const;
    int accelDirectionUp() const;
    int accelDirectionDown() const;
    int accelDirectionForward() const;
    int accelDirectionBackward() const;
    int accelDirectionLeft() const;
    int accelDirectionRight() const;
    int commandRebootCid0() const;
    int commandRebootCid1() const;
    int commandRebootCid2() const;
    int commandToggleMotorTestCid0() const;
    int commandToggleMotorTestCid1() const;
    int commandToggleMotorTestCid2() const;
    int commandMotorTestThrottleCid0() const;
    int commandMotorTestThrottleCid1() const;
    int commandMotorTestThrottleCid2() const;
    int framePwm() const;
    int frameCustomSystemInfo() const;
    int frameCustomPid() const;
    int flightStatusMotorTest() const;

    Q_INVOKABLE QVariantList commandFramePayloads(const QVector<_un_anotc_v8_frame> &frames) const;

private:
    static int cid0(quint32 commandId);
    static int cid1(quint32 commandId);
    static int cid2(quint32 commandId);
};
