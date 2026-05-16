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

public:
    explicit Flight(QObject *parent = nullptr);

    bool imu() const;
    bool mag() const;
    bool baro() const;

public slots:
    void reset();
    void setImu(bool on);
    void setMag(bool on);
    void setBaro(bool on);

signals:
    void imgStatusChanged();
    void magStatusChanged();
    void baroStatusChanged();

private:
    bool m_imu = false;
    bool m_mag = false;
    bool m_baro = false;
};
