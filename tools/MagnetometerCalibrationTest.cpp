#include "../src/app/MagnetometerCalibration.h"

#include <QCoreApplication>
#include <QVariantMap>
#include <QVector>
#include <QtEndian>

#include <array>
#include <cmath>
#include <iostream>

namespace {

using Sample = MagnetometerCalibration::Sample;

constexpr double Pi = 3.14159265358979323846;

std::array<double, 3> multiply(const std::array<double, 9> &matrix,
                               const std::array<double, 3> &vector)
{
    return {
        matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2],
        matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2],
        matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2]
    };
}

bool closeTo(double actual, double expected, double tolerance, const char *label)
{
    if (std::abs(actual - expected) <= tolerance) {
        return true;
    }

    std::cerr << label << ": expected " << expected << ", got " << actual << '\n';
    return false;
}

_un_anotc_v8_frame makeMagnetometerFrame(const Sample &sample)
{
    _un_anotc_v8_frame frame {};
    frame.frame.fun = ANOTC_FRAME_MAG;
    frame.frame.len = 6;
    qToLittleEndian<qint16>(static_cast<qint16>(std::lround(sample.x)), frame.frame.data);
    qToLittleEndian<qint16>(static_cast<qint16>(std::lround(sample.y)), frame.frame.data + 2);
    qToLittleEndian<qint16>(static_cast<qint16>(std::lround(sample.z)), frame.frame.data + 4);
    return frame;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const std::array<double, 9> distortion {
        1.30, 0.10, -0.04,
        0.10, 0.78, 0.06,
        -0.04, 0.06, 1.08
    };
    const std::array<double, 3> offsets {42.0, -17.0, 9.0};
    constexpr double fieldStrength = 57.0;

    QVector<Sample> samples;
    for (int elevationIndex = 0; elevationIndex <= 12; ++elevationIndex) {
        const double elevation = -0.5 * Pi + Pi * elevationIndex / 12.0;
        for (int azimuthIndex = 0; azimuthIndex < 24; ++azimuthIndex) {
            const double azimuth = 2.0 * Pi * azimuthIndex / 24.0;
            const std::array<double, 3> sphere {
                fieldStrength * std::cos(elevation) * std::cos(azimuth),
                fieldStrength * std::cos(elevation) * std::sin(azimuth),
                fieldStrength * std::sin(elevation)
            };
            const std::array<double, 3> distorted = multiply(distortion, sphere);
            samples.append({
                distorted[0] + offsets[0],
                distorted[1] + offsets[1],
                distorted[2] + offsets[2]
            });
        }
    }

    MagnetometerCalibration::CalibrationResult result;
    QString error;
    if (!MagnetometerCalibration::fitSamples(samples, fieldStrength, &result, &error)) {
        std::cerr << "fit failed: " << error.toStdString() << '\n';
        return 1;
    }

    bool passed = true;
    for (int axis = 0; axis < 3; ++axis) {
        passed &= closeTo(result.hardIronOffsets[axis], offsets[axis], 1.0e-6, "offset");
    }
    passed &= closeTo(result.residualRmsPercent, 0.0, 1.0e-6, "residual");

    double calibratedSquaredError = 0.0;
    for (const Sample &sample : samples) {
        const std::array<double, 3> centered {
            sample.x - result.hardIronOffsets[0],
            sample.y - result.hardIronOffsets[1],
            sample.z - result.hardIronOffsets[2]
        };
        const std::array<double, 3> calibrated = multiply(result.softIronMatrix, centered);
        const double magnitude = std::sqrt(calibrated[0] * calibrated[0]
                                           + calibrated[1] * calibrated[1]
                                           + calibrated[2] * calibrated[2]);
        calibratedSquaredError += (magnitude - fieldStrength) * (magnitude - fieldStrength);
    }
    passed &= closeTo(std::sqrt(calibratedSquaredError / samples.size()), 0.0, 1.0e-6, "calibrated RMS");

    MagnetometerCalibration collector;
    collector.startCollection();
    for (const Sample &sample : samples) {
        collector.processFrames({makeMagnetometerFrame(sample)});
    }
    collector.stopCollection();
    if (collector.sampleCount() < 100) {
        std::cerr << "telemetry collector accepted too few samples: " << collector.sampleCount() << '\n';
        passed = false;
    }
    if (collector.coveragePercent() < 80) {
        std::cerr << "telemetry collector coverage too low: " << collector.coveragePercent() << "%\n";
        passed = false;
    }
    if (!collector.calculate()) {
        std::cerr << "telemetry collector fit failed: " << collector.lastError().toStdString() << '\n';
        passed = false;
    }
    if (collector.rawPointCloud().isEmpty() || collector.rawPointCloud().size() > 900) {
        std::cerr << "raw point cloud has invalid size: " << collector.rawPointCloud().size() << '\n';
        passed = false;
    }
    const QVariantMap firstRawPoint = collector.rawPointCloud().constFirst().toMap();
    if (!firstRawPoint.contains(QStringLiteral("x"))
        || !firstRawPoint.contains(QStringLiteral("y"))
        || !firstRawPoint.contains(QStringLiteral("z"))) {
        std::cerr << "raw point cloud entry does not expose named coordinates\n";
        passed = false;
    }
    if (collector.calibratedPointCloud().size() != collector.rawPointCloud().size()) {
        std::cerr << "calibrated point cloud does not match raw point count\n";
        passed = false;
    }

    if (!passed) {
        return 1;
    }

    std::cout << "magnetometer calibration fit passed\n";
    return 0;
}
