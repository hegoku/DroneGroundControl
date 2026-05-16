#include "Flight.h"

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

void Flight::reset()
{
    setImu(false);
    setMag(false);
    setBaro(false);
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
