#include "ConnectionSessionBridge.h"

#include "../protocol/AnotcCmdFrame.h"

#include <QByteArray>
#include <QVariantMap>

ConnectionSessionBridge::ConnectionSessionBridge(QObject *parent)
    : QObject(parent)
{
}

int ConnectionSessionBridge::frameCmdSend() const
{
    return ANOTC_FRAME_CMD_SEND;
}

int ConnectionSessionBridge::commandCalibrateGyro() const
{
    return static_cast<int>(ANOTC_CMD_CALIBRATE_GYRO);
}

int ConnectionSessionBridge::commandCalibrateGyroCid0() const
{
    return cid0(ANOTC_CMD_CALIBRATE_GYRO);
}

int ConnectionSessionBridge::commandCalibrateGyroCid1() const
{
    return cid1(ANOTC_CMD_CALIBRATE_GYRO);
}

int ConnectionSessionBridge::commandCalibrateGyroCid2() const
{
    return cid2(ANOTC_CMD_CALIBRATE_GYRO);
}

int ConnectionSessionBridge::commandCalibrateAccel() const
{
    return static_cast<int>(ANOTC_CMD_CALIBRATE_ACCEL);
}

int ConnectionSessionBridge::commandCalibrateAccelCid0() const
{
    return cid0(ANOTC_CMD_CALIBRATE_ACCEL);
}

int ConnectionSessionBridge::commandCalibrateAccelCid1() const
{
    return cid1(ANOTC_CMD_CALIBRATE_ACCEL);
}

int ConnectionSessionBridge::commandCalibrateAccelCid2() const
{
    return cid2(ANOTC_CMD_CALIBRATE_ACCEL);
}

int ConnectionSessionBridge::accelDirectionStart() const
{
    return 0;
}

int ConnectionSessionBridge::accelDirectionUp() const
{
    return 'U';
}

int ConnectionSessionBridge::accelDirectionDown() const
{
    return 'D';
}

int ConnectionSessionBridge::accelDirectionForward() const
{
    return 'F';
}

int ConnectionSessionBridge::accelDirectionBackward() const
{
    return 'B';
}

int ConnectionSessionBridge::accelDirectionLeft() const
{
    return 'L';
}

int ConnectionSessionBridge::accelDirectionRight() const
{
    return 'R';
}

int ConnectionSessionBridge::commandRebootCid0() const
{
    return cid0(ANOTC_CMD_REBOOT);
}

int ConnectionSessionBridge::commandRebootCid1() const
{
    return cid1(ANOTC_CMD_REBOOT);
}

int ConnectionSessionBridge::commandRebootCid2() const
{
    return cid2(ANOTC_CMD_REBOOT);
}

int ConnectionSessionBridge::commandToggleMotorTestCid0() const
{
    return cid0(ANOTC_CMD_TOGGLE_MOTOR_TEST_STATUS);
}

int ConnectionSessionBridge::commandToggleMotorTestCid1() const
{
    return cid1(ANOTC_CMD_TOGGLE_MOTOR_TEST_STATUS);
}

int ConnectionSessionBridge::commandToggleMotorTestCid2() const
{
    return cid2(ANOTC_CMD_TOGGLE_MOTOR_TEST_STATUS);
}

int ConnectionSessionBridge::commandMotorTestThrottleCid0() const
{
    return cid0(ANOTC_CMD_MOTOR_TEST_THROTTLE);
}

int ConnectionSessionBridge::commandMotorTestThrottleCid1() const
{
    return cid1(ANOTC_CMD_MOTOR_TEST_THROTTLE);
}

int ConnectionSessionBridge::commandMotorTestThrottleCid2() const
{
    return cid2(ANOTC_CMD_MOTOR_TEST_THROTTLE);
}

int ConnectionSessionBridge::framePwm() const
{
    return ANOTC_FRAME_PWM;
}

int ConnectionSessionBridge::frameCustomSystemInfo() const
{
    return ANOTC_FRAME_CUSTOM_SYSTEM_INFO;
}

int ConnectionSessionBridge::frameCustomPid() const
{
    return ANOTC_FRAME_CUSTOM_PID;
}

int ConnectionSessionBridge::flightStatusMotorTest() const
{
    return 13;
}

QVariantList ConnectionSessionBridge::commandFramePayloads(const QVector<_un_anotc_v8_frame> &frames) const
{
    QVariantList payloads;
    payloads.reserve(frames.size());

    for (const _un_anotc_v8_frame &frame : frames) {
        QVariantMap map;
        const quint16 length = frame.frame.len;
        const QByteArray payload(reinterpret_cast<const char *>(frame.frame.data), length);

        map.insert(QStringLiteral("function"), frame.frame.fun);
        map.insert(QStringLiteral("length"), length);
        map.insert(QStringLiteral("payloadHex"), QString::fromLatin1(payload.toHex(' ').toUpper()));
        payloads.append(map);
    }

    return payloads;
}

int ConnectionSessionBridge::cid0(quint32 commandId)
{
    return static_cast<int>((commandId >> 16) & 0xFF);
}

int ConnectionSessionBridge::cid1(quint32 commandId)
{
    return static_cast<int>((commandId >> 8) & 0xFF);
}

int ConnectionSessionBridge::cid2(quint32 commandId)
{
    return static_cast<int>(commandId & 0xFF);
}
