#include "../src/app/MagnetometerCalibration.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include <iostream>

namespace {

bool readSamples(const QString &path, QVector<MagnetometerCalibration::Sample> *samples)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "failed to open input file: " << path.toStdString() << '\n';
        return false;
    }

    QTextStream stream(&file);
    int lineNumber = 0;
    while (!stream.atEnd()) {
        ++lineNumber;
        QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QTextStream lineStream(&line, QIODevice::ReadOnly);
        MagnetometerCalibration::Sample sample;
        lineStream >> sample.x >> sample.y >> sample.z;
        if (lineStream.status() != QTextStream::Ok) {
            std::cerr << "invalid XYZ sample at line " << lineNumber << '\n';
            return false;
        }
        samples->append(sample);
    }
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    if (argc != 2 && argc != 4) {
        std::cerr << "usage: magnetometer_calibration_cli <xyz-sample-file> [--norm <value>]\n";
        return 2;
    }

    double requestedFieldStrength = 0.0;
    if (argc == 4) {
        bool ok = false;
        requestedFieldStrength = QString::fromLocal8Bit(argv[3]).toDouble(&ok);
        if (QString::fromLocal8Bit(argv[2]) != QStringLiteral("--norm")
            || !ok
            || requestedFieldStrength <= 0.0) {
            std::cerr << "--norm must be a positive number\n";
            return 2;
        }
    }

    QVector<MagnetometerCalibration::Sample> samples;
    if (!readSamples(QString::fromLocal8Bit(argv[1]), &samples)) {
        return 1;
    }

    MagnetometerCalibration::CalibrationResult result;
    QString error;
    if (!MagnetometerCalibration::fitSamples(samples, requestedFieldStrength, &result, &error)) {
        std::cerr << "calibration failed: " << error.toStdString() << '\n';
        return 1;
    }

    std::cout.precision(12);
    std::cout << "samples: " << samples.size() << '\n';
    std::cout << "target_field_strength: " << result.targetFieldStrength << '\n';
    std::cout << "residual_rms_percent: " << result.residualRmsPercent << '\n';
    std::cout << "hard_iron_offsets:\n";
    std::cout << result.hardIronOffsets[0] << ' '
              << result.hardIronOffsets[1] << ' '
              << result.hardIronOffsets[2] << '\n';
    std::cout << "soft_iron_matrix:\n";
    for (int row = 0; row < 3; ++row) {
        std::cout << result.softIronMatrix[row * 3] << ' '
                  << result.softIronMatrix[row * 3 + 1] << ' '
                  << result.softIronMatrix[row * 3 + 2] << '\n';
    }
    return 0;
}
