#pragma once

#include <QMetaType>
#include <QtGlobal>
#include <QVector>

#define ANOTC_DATA_MAX_SIZE 512
#define ANOTC_V8_HEAD_SIZE 6
#define ANOTC_V8_HEAD 0xAB

#define ANOTC_FRAME_FRAME_CHECK 0x00
#define ANOTC_FRAME_IMU 0x01
#define ANOTC_FRAME_MAG 0x02
#define ANOTC_FRAME_EULER 0x03
#define ANOTC_FRAME_QUAT 0x04
#define ANOTC_FRAME_ALT 0x05
#define ANOTC_FRAME_SPEED 0x07
#define ANOTC_FRAME_TARGET_ATTITUDE 0x0A
#define ANOTC_FRAME_BATTERY 0x0D
#define ANOTC_FRAME_PWM 0x20
#define ANOTC_FRAME_GPS 0x30
#define ANOTC_FRAME_RC 0x40
#define ANOTC_FRAME_LOG_STRING 0xA0
#define ANOTC_FRAME_LOG_STRING_NUM 0xA1
#define ANOTC_FRAME_CMD_SEND 0xC0
#define ANOTC_FRAME_CONFIG_CMD 0xE0
#define ANOTC_FRAME_CONFIG_READ_WRITE 0xE1
#define ANOTC_FRAME_CONFIG_INFO 0xE2
#define ANOTC_FRAME_DEVICE_INFO 0xE3
#define ANOTC_FRAME_CUSTOM_SYSTEM_INFO 0xF1
#define ANOTC_FRAME_CUSTOM_PID 0xF2

// config frame cmd
#define ANOTC_CONFIG_FRAME_CMD_DEVICE_INFO 0x00
#define ANOTC_CONFIG_FRAME_CMD_READ_COUNT 0x01
#define ANOTC_CONFIG_FRAME_CMD_READ_VALUE 0x02
#define ANOTC_CONFIG_FRAME_CMD_READ_INFO 0x03
#define ANOTC_CONFIG_FRAME_CMD_0X10 0x10

#define ANOTC_CONFIG_FRAME_VAL_RESET_PARAM 0xAA
#define ANOTC_CONFIG_FRAME_VAL_SAVE_PARAM 0xAB

#pragma pack(push, 1)
struct anotc_frame
{
    quint8 head;
    quint8 saddr;
    quint8 daddr;
    quint8 fun;
    quint16 len;
    quint8 data[ANOTC_DATA_MAX_SIZE];
};
#pragma pack(pop)

union _un_anotc_v8_frame
{
    anotc_frame frame;
    quint8 rawBytes[sizeof(anotc_frame)];
};

struct AnotcParseResult
{
    QVector<_un_anotc_v8_frame> telemetryFrames;
    QVector<_un_anotc_v8_frame> parameterFrames;
    QVector<_un_anotc_v8_frame> ackFrames;
    QVector<_un_anotc_v8_frame> commandFrames;
    QVector<_un_anotc_v8_frame> logFrames;
    QVector<_un_anotc_v8_frame> unknownFrames;

    bool isEmpty() const
    {
        return telemetryFrames.isEmpty()
            && parameterFrames.isEmpty()
            && ackFrames.isEmpty()
            && commandFrames.isEmpty()
            && logFrames.isEmpty()
            && unknownFrames.isEmpty();
    }

    qsizetype totalCount() const
    {
        return telemetryFrames.size()
            + parameterFrames.size()
            + ackFrames.size()
            + commandFrames.size()
            + logFrames.size()
            + unknownFrames.size();
    }

    void reserveTelemetry(qsizetype count)
    {
        telemetryFrames.reserve(count);
    }
};

Q_DECLARE_METATYPE(_un_anotc_v8_frame)
Q_DECLARE_METATYPE(QVector<_un_anotc_v8_frame>)
Q_DECLARE_METATYPE(AnotcParseResult)
