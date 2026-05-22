#pragma once

#include "../protocol/AnotcFrame.h"

#include <QObject>
#include <QtGlobal>

class Flight : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool imu READ imu WRITE setImu NOTIFY imgStatusChanged)
    Q_PROPERTY(bool mag READ mag WRITE setMag NOTIFY magStatusChanged)
    Q_PROPERTY(bool baro READ baro WRITE setBaro NOTIFY baroStatusChanged)
    Q_PROPERTY(double voltage READ voltage NOTIFY voltageChanged)
    Q_PROPERTY(int battery_count READ batteryCount WRITE setBatteryCount NOTIFY batteryCountChanged)

    enum flight_status {
        FLIGHT_STATUS_SELFTEST,
        FLIGHT_STATUS_READY,
        FLIGHT_STATUS_ANGLE_RATE_MODE,
        FLIGHT_STATUS_ANGLE_MODE,
        FLIGHT_STATUS_CALIBRATION_ACCEL,
        FLIGHT_STATUS_CALIBRATION_ACCEL_UP,
        FLIGHT_STATUS_CALIBRATION_ACCEL_DOWN,
        FLIGHT_STATUS_CALIBRATION_ACCEL_FORWARD,
        FLIGHT_STATUS_CALIBRATION_ACCEL_BACKWARD,
        FLIGHT_STATUS_CALIBRATION_ACCEL_LEFT,
        FLIGHT_STATUS_CALIBRATION_ACCEL_RIGHT,
        FLIGHT_STATUS_CALIBRATION_GYRO,
        FLIGHT_STATUS_CALIBRATION_COMPASS,
        FLIGHT_STATUS_MOTOR_TEST,
        FLIGHT_STATUS_SELFTEST_FAILED
    };

public:
    explicit Flight(QObject *parent = nullptr);

    bool imu() const;
    bool mag() const;
    bool baro() const;
    double voltage() const;
    int batteryCount() const;

public slots:
    void reset();
    void setImu(bool on);
    void setMag(bool on);
    void setBaro(bool on);
    void setBatteryCount(int count);
    void processFrames(const QVector<_un_anotc_v8_frame> &frames);

signals:
    void imgStatusChanged();
    void magStatusChanged();
    void baroStatusChanged();
    void voltageChanged();
    void batteryCountChanged();

private:
    void setVoltage(double voltage);

    bool m_imu = false;
    bool m_mag = false;
    bool m_baro = false;
    double m_voltage = 0.0;
    int m_batteryCount = 1;
};
