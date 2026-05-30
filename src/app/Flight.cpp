#include "Flight.h"

#include <QtEndian>
#include <QtMath>

Flight::Flight(QObject *parent)
    : QObject(parent)
{
}

bool Flight::imu() const
{
    return m_imu;
}

bool Flight::mag() const
{
    return m_mag;
}

bool Flight::baro() const
{
    return m_baro;
}

double Flight::voltage() const
{
    return m_voltage;
}

int Flight::batteryCount() const
{
    return m_batteryCount;
}

Flight::flight_status Flight::status() const
{
    return m_status;
}

unsigned char Flight::cpuLoad() const
{
    return m_cpuLoad;
}

double Flight::roll() const
{
    return m_roll;
}

double Flight::pitch() const
{
    return m_pitch;
}

double Flight::yaw() const
{
    return m_yaw;
}

void Flight::reset()
{
    setImu(false);
    setMag(false);
    setBaro(false);
    setStatus(FLIGHT_STATUS_SELFTEST);
    setCpuLoad(0);
    setAttitude(0.0, 0.0, 0.0);
    if (!qFuzzyIsNull(m_voltage)) {
        m_voltage = 0.0;
        emit voltageChanged();
    }
}

void Flight::setImu(bool on)
{
    m_imu = on;
    emit imgStatusChanged();
}

void Flight::setMag(bool on)
{
    m_mag = on;
    emit magStatusChanged();
}

void Flight::setBaro(bool on)
{
    m_baro = on;
    emit baroStatusChanged();
}

void Flight::setBatteryCount(int count)
{
    const int normalizedCount = qMax(1, count);
    if (m_batteryCount == normalizedCount) {
        return;
    }

    m_batteryCount = normalizedCount;
    emit batteryCountChanged();
}

void Flight::processFrames(const QVector<_un_anotc_v8_frame> &frames)
{
    for (const _un_anotc_v8_frame &frame : frames) {
        if (frame.frame.fun == ANOTC_FRAME_EULER && frame.frame.len >= 6) {
            const double roll = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data)) / 100.0;
            const double pitch = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data + 2)) / 100.0;
            const double yaw = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data + 4)) / 100.0;
            setAttitude(roll, pitch, yaw);
            continue;
        }

        if (frame.frame.fun != ANOTC_FRAME_CUSTOM_SYSTEM_INFO || frame.frame.len < 7) {
            continue;
        }

        setStatus(static_cast<flight_status>(frame.frame.data[0]));
        setCpuLoad(frame.frame.data[1]);

        const quint16 rawVoltage = qFromLittleEndian<quint16>(frame.frame.data + 2);
        setVoltage(static_cast<double>(rawVoltage) / 100.0);
    }
}

void Flight::setVoltage(double voltage)
{
    if (qFuzzyCompare(m_voltage + 1.0, voltage + 1.0)) {
        return;
    }

    m_voltage = voltage;
    emit voltageChanged();
}

void Flight::setStatus(flight_status status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    emit statusChanged();
}

void Flight::setCpuLoad(unsigned char cpuLoad)
{
    if (m_cpuLoad == cpuLoad) {
        return;
    }

    m_cpuLoad = cpuLoad;
    emit cpuLoadChanged();
}

void Flight::setAttitude(double roll, double pitch, double yaw)
{
    if (qFuzzyCompare(m_roll + 1.0, roll + 1.0)
        && qFuzzyCompare(m_pitch + 1.0, pitch + 1.0)
        && qFuzzyCompare(m_yaw + 1.0, yaw + 1.0)) {
        return;
    }

    m_roll = roll;
    m_pitch = pitch;
    m_yaw = yaw;
    emit attitudeChanged();
}
