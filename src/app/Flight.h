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
