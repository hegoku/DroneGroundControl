#pragma once

#include "../protocol/AnotcFrame.h"

#include <QObject>
#include <QVariantList>
#include <QVector>

#include <array>

class MagnetometerCalibration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool collecting READ collecting NOTIFY collectingChanged)
    Q_PROPERTY(int sampleCount READ sampleCount NOTIFY sampleCountChanged)
    Q_PROPERTY(int coveragePercent READ coveragePercent NOTIFY coveragePercentChanged)
    Q_PROPERTY(bool calibrationValid READ calibrationValid NOTIFY calibrationChanged)
    Q_PROPERTY(QVariantList softIronMatrix READ softIronMatrix NOTIFY calibrationChanged)
    Q_PROPERTY(QVariantList hardIronOffsets READ hardIronOffsets NOTIFY calibrationChanged)
    Q_PROPERTY(double targetFieldStrength READ targetFieldStrength NOTIFY calibrationChanged)
    Q_PROPERTY(double residualRmsPercent READ residualRmsPercent NOTIFY calibrationChanged)
    Q_PROPERTY(QVariantList rawPointCloud READ rawPointCloud NOTIFY pointCloudChanged)
    Q_PROPERTY(QVariantList calibratedPointCloud READ calibratedPointCloud NOTIFY pointCloudChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    struct Sample
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    struct CalibrationResult
    {
        std::array<double, 9> softIronMatrix {};
        std::array<double, 3> hardIronOffsets {};
        double targetFieldStrength = 0.0;
        double residualRmsPercent = 0.0;
    };

    explicit MagnetometerCalibration(QObject *parent = nullptr);

    bool collecting() const;
    int sampleCount() const;
    int coveragePercent() const;
    bool calibrationValid() const;
    QVariantList softIronMatrix() const;
    QVariantList hardIronOffsets() const;
    double targetFieldStrength() const;
    double residualRmsPercent() const;
    QVariantList rawPointCloud() const;
    QVariantList calibratedPointCloud() const;
    QString lastError() const;

    Q_INVOKABLE void startCollection();
    Q_INVOKABLE void stopCollection();
    Q_INVOKABLE void clearSamples();
    Q_INVOKABLE bool calculate(double requestedFieldStrength = 0.0);

    static bool fitSamples(const QVector<Sample> &samples,
                           double requestedFieldStrength,
                           CalibrationResult *result,
                           QString *error);

public slots:
    void processFrames(const QVector<_un_anotc_v8_frame> &frames);

signals:
    void collectingChanged();
    void sampleCountChanged();
    void coveragePercentChanged();
    void calibrationChanged();
    void pointCloudChanged();
    void lastErrorChanged();

private:
    static constexpr int MaximumSampleCount = 10000;
    static constexpr int MaximumRenderedPointCount = 900;

    QVariantList pointCloud(bool calibrated) const;
    void appendSample(const Sample &sample);
    void updateCoverage();
    void invalidateCalibration();
    void setCollecting(bool collecting);
    void setCoveragePercent(int coveragePercent);
    void setLastError(const QString &lastError);

    QVector<Sample> m_samples;
    CalibrationResult m_result;
    Sample m_lastAcceptedSample;
    bool m_hasLastAcceptedSample = false;
    bool m_collecting = false;
    bool m_calibrationValid = false;
    int m_coveragePercent = 0;
    QString m_lastError;
};
