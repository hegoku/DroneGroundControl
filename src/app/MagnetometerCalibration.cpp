#include "MagnetometerCalibration.h"

#include <QtEndian>
#include <QVariantMap>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace {

template<int Size>
using Vector = std::array<double, Size>;

template<int Rows, int Columns>
using Matrix = std::array<std::array<double, Columns>, Rows>;

using Matrix3 = Matrix<3, 3>;
using Matrix9 = Matrix<9, 9>;
using Vector3 = Vector<3>;
using Vector9 = Vector<9>;

template<int Size>
bool solveLinearSystem(Matrix<Size, Size> matrix, Vector<Size> values, Vector<Size> *solution)
{
    double largestCoefficient = 0.0;
    for (const auto &row : matrix) {
        for (const double value : row) {
            largestCoefficient = std::max(largestCoefficient, std::abs(value));
        }
    }
    const double minimumPivot = std::max(1.0, largestCoefficient) * 1.0e-12;

    for (int pivot = 0; pivot < Size; ++pivot) {
        int pivotRow = pivot;
        double pivotMagnitude = std::abs(matrix[pivot][pivot]);
        for (int row = pivot + 1; row < Size; ++row) {
            const double magnitude = std::abs(matrix[row][pivot]);
            if (magnitude > pivotMagnitude) {
                pivotMagnitude = magnitude;
                pivotRow = row;
            }
        }

        if (pivotMagnitude <= minimumPivot) {
            return false;
        }
        if (pivotRow != pivot) {
            std::swap(matrix[pivotRow], matrix[pivot]);
            std::swap(values[pivotRow], values[pivot]);
        }

        const double pivotValue = matrix[pivot][pivot];
        for (int row = pivot + 1; row < Size; ++row) {
            const double factor = matrix[row][pivot] / pivotValue;
            matrix[row][pivot] = 0.0;
            for (int column = pivot + 1; column < Size; ++column) {
                matrix[row][column] -= factor * matrix[pivot][column];
            }
            values[row] -= factor * values[pivot];
        }
    }

    for (int row = Size - 1; row >= 0; --row) {
        double value = values[row];
        for (int column = row + 1; column < Size; ++column) {
            value -= matrix[row][column] * (*solution)[column];
        }
        (*solution)[row] = value / matrix[row][row];
    }
    return true;
}

bool invertMatrix3(const Matrix3 &matrix, Matrix3 *inverse)
{
    for (int column = 0; column < 3; ++column) {
        Vector3 unit {};
        unit[column] = 1.0;
        Vector3 solution {};
        if (!solveLinearSystem<3>(matrix, unit, &solution)) {
            return false;
        }
        for (int row = 0; row < 3; ++row) {
            (*inverse)[row][column] = solution[row];
        }
    }
    return true;
}

Vector3 multiply(const Matrix3 &matrix, const Vector3 &vector)
{
    Vector3 result {};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result[row] += matrix[row][column] * vector[column];
        }
    }
    return result;
}

double dot(const Vector3 &left, const Vector3 &right)
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

double determinant(const Matrix3 &matrix)
{
    return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
        - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
        + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

Matrix3 identityMatrix3()
{
    Matrix3 identity {};
    for (int index = 0; index < 3; ++index) {
        identity[index][index] = 1.0;
    }
    return identity;
}

bool symmetricEigenDecomposition(Matrix3 matrix, Vector3 *eigenvalues, Matrix3 *eigenvectors)
{
    *eigenvectors = identityMatrix3();

    for (int iteration = 0; iteration < 40; ++iteration) {
        int pivotRow = 0;
        int pivotColumn = 1;
        double largestOffDiagonal = std::abs(matrix[0][1]);
        for (int row = 0; row < 3; ++row) {
            for (int column = row + 1; column < 3; ++column) {
                const double magnitude = std::abs(matrix[row][column]);
                if (magnitude > largestOffDiagonal) {
                    largestOffDiagonal = magnitude;
                    pivotRow = row;
                    pivotColumn = column;
                }
            }
        }

        double largestDiagonal = 0.0;
        for (int index = 0; index < 3; ++index) {
            largestDiagonal = std::max(largestDiagonal, std::abs(matrix[index][index]));
        }
        if (largestOffDiagonal <= std::max(1.0, largestDiagonal) * 1.0e-14) {
            break;
        }

        const double app = matrix[pivotRow][pivotRow];
        const double aqq = matrix[pivotColumn][pivotColumn];
        const double apq = matrix[pivotRow][pivotColumn];
        const double angle = 0.5 * std::atan2(2.0 * apq, aqq - app);
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);

        for (int index = 0; index < 3; ++index) {
            if (index == pivotRow || index == pivotColumn) {
                continue;
            }
            const double aip = matrix[index][pivotRow];
            const double aiq = matrix[index][pivotColumn];
            matrix[index][pivotRow] = cosine * aip - sine * aiq;
            matrix[pivotRow][index] = matrix[index][pivotRow];
            matrix[index][pivotColumn] = sine * aip + cosine * aiq;
            matrix[pivotColumn][index] = matrix[index][pivotColumn];
        }

        matrix[pivotRow][pivotRow] = cosine * cosine * app
            - 2.0 * sine * cosine * apq
            + sine * sine * aqq;
        matrix[pivotColumn][pivotColumn] = sine * sine * app
            + 2.0 * sine * cosine * apq
            + cosine * cosine * aqq;
        matrix[pivotRow][pivotColumn] = 0.0;
        matrix[pivotColumn][pivotRow] = 0.0;

        for (int row = 0; row < 3; ++row) {
            const double vip = (*eigenvectors)[row][pivotRow];
            const double viq = (*eigenvectors)[row][pivotColumn];
            (*eigenvectors)[row][pivotRow] = cosine * vip - sine * viq;
            (*eigenvectors)[row][pivotColumn] = sine * vip + cosine * viq;
        }
    }

    for (int index = 0; index < 3; ++index) {
        (*eigenvalues)[index] = matrix[index][index];
        if (!std::isfinite((*eigenvalues)[index])) {
            return false;
        }
    }
    return true;
}

Matrix3 symmetricSquareRoot(const Vector3 &eigenvalues, const Matrix3 &eigenvectors)
{
    Matrix3 result {};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            for (int eigenvalue = 0; eigenvalue < 3; ++eigenvalue) {
                result[row][column] += eigenvectors[row][eigenvalue]
                    * std::sqrt(eigenvalues[eigenvalue])
                    * eigenvectors[column][eigenvalue];
            }
        }
    }
    return result;
}

Vector3 toVector(const MagnetometerCalibration::Sample &sample)
{
    return {sample.x, sample.y, sample.z};
}

double distanceSquared(const MagnetometerCalibration::Sample &left,
                       const MagnetometerCalibration::Sample &right)
{
    const double dx = left.x - right.x;
    const double dy = left.y - right.y;
    const double dz = left.z - right.z;
    return dx * dx + dy * dy + dz * dz;
}

} // namespace

MagnetometerCalibration::MagnetometerCalibration(QObject *parent)
    : QObject(parent)
{
}

bool MagnetometerCalibration::collecting() const
{
    return m_collecting;
}

int MagnetometerCalibration::sampleCount() const
{
    return m_samples.size();
}

int MagnetometerCalibration::coveragePercent() const
{
    return m_coveragePercent;
}

bool MagnetometerCalibration::calibrationValid() const
{
    return m_calibrationValid;
}

QVariantList MagnetometerCalibration::softIronMatrix() const
{
    QVariantList values;
    values.reserve(m_result.softIronMatrix.size());
    for (const double value : m_result.softIronMatrix) {
        values.append(value);
    }
    return values;
}

QVariantList MagnetometerCalibration::hardIronOffsets() const
{
    QVariantList values;
    values.reserve(m_result.hardIronOffsets.size());
    for (const double value : m_result.hardIronOffsets) {
        values.append(value);
    }
    return values;
}

double MagnetometerCalibration::targetFieldStrength() const
{
    return m_result.targetFieldStrength;
}

double MagnetometerCalibration::residualRmsPercent() const
{
    return m_result.residualRmsPercent;
}

QVariantList MagnetometerCalibration::rawPointCloud() const
{
    return pointCloud(false);
}

QVariantList MagnetometerCalibration::calibratedPointCloud() const
{
    return pointCloud(true);
}

QString MagnetometerCalibration::lastError() const
{
    return m_lastError;
}

void MagnetometerCalibration::startCollection()
{
    clearSamples();
    setCollecting(true);
}

void MagnetometerCalibration::stopCollection()
{
    setCollecting(false);
}

void MagnetometerCalibration::clearSamples()
{
    const bool hadSamples = !m_samples.isEmpty();
    m_samples.clear();
    m_hasLastAcceptedSample = false;
    invalidateCalibration();
    setCoveragePercent(0);
    setLastError({});
    if (hadSamples) {
        emit sampleCountChanged();
        emit pointCloudChanged();
    }
}

bool MagnetometerCalibration::calculate(double requestedFieldStrength)
{
    CalibrationResult result;
    QString error;
    if (!fitSamples(m_samples, requestedFieldStrength, &result, &error)) {
        invalidateCalibration();
        setLastError(error);
        return false;
    }

    m_result = result;
    m_calibrationValid = true;
    setLastError({});
    emit calibrationChanged();
    emit pointCloudChanged();
    return true;
}

bool MagnetometerCalibration::fitSamples(const QVector<Sample> &samples,
                                         double requestedFieldStrength,
                                         CalibrationResult *result,
                                         QString *error)
{
    if (!result) {
        return false;
    }
    if (!std::isfinite(requestedFieldStrength) || requestedFieldStrength < 0.0) {
        if (error) {
            *error = QStringLiteral("Reference field strength must be positive or zero for auto.");
        }
        return false;
    }
    if (samples.size() < 100) {
        if (error) {
            *error = QStringLiteral("Collect at least 100 distinct samples before calculating.");
        }
        return false;
    }

    Vector3 mean {};
    Vector3 minimum {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };
    Vector3 maximum {
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()
    };
    for (const Sample &sample : samples) {
        const Vector3 value = toVector(sample);
        for (int axis = 0; axis < 3; ++axis) {
            mean[axis] += value[axis];
            minimum[axis] = std::min(minimum[axis], value[axis]);
            maximum[axis] = std::max(maximum[axis], value[axis]);
        }
    }
    for (double &value : mean) {
        value /= samples.size();
    }

    Vector3 spans {};
    for (int axis = 0; axis < 3; ++axis) {
        spans[axis] = maximum[axis] - minimum[axis];
    }
    const double largestSpan = *std::max_element(spans.begin(), spans.end());
    const double smallestSpan = *std::min_element(spans.begin(), spans.end());
    if (!std::isfinite(largestSpan) || largestSpan <= 0.0 || smallestSpan < largestSpan * 0.25) {
        if (error) {
            *error = QStringLiteral("Samples do not cover all three axes. Rotate the aircraft through more orientations.");
        }
        return false;
    }

    double inputScaleSquared = 0.0;
    for (const Sample &sample : samples) {
        const Vector3 value = toVector(sample);
        for (int axis = 0; axis < 3; ++axis) {
            const double centered = value[axis] - mean[axis];
            inputScaleSquared += centered * centered;
        }
    }
    const double inputScale = std::sqrt(inputScaleSquared / samples.size());
    if (!std::isfinite(inputScale) || inputScale <= std::numeric_limits<double>::epsilon()) {
        if (error) {
            *error = QStringLiteral("Samples have no usable range.");
        }
        return false;
    }

    // Fit x^T Q x + 2 u^T x = 1 after normalization to keep the least-squares
    // system well-conditioned for both raw counts and physical field units.
    Matrix9 normalMatrix {};
    Vector9 normalValues {};
    for (const Sample &sample : samples) {
        const double x = (sample.x - mean[0]) / inputScale;
        const double y = (sample.y - mean[1]) / inputScale;
        const double z = (sample.z - mean[2]) / inputScale;
        const Vector9 design {
            x * x,
            y * y,
            z * z,
            2.0 * x * y,
            2.0 * x * z,
            2.0 * y * z,
            2.0 * x,
            2.0 * y,
            2.0 * z
        };

        for (int row = 0; row < 9; ++row) {
            normalValues[row] += design[row];
            for (int column = 0; column < 9; ++column) {
                normalMatrix[row][column] += design[row] * design[column];
            }
        }
    }

    Vector9 coefficients {};
    if (!solveLinearSystem<9>(normalMatrix, normalValues, &coefficients)) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit is singular. Collect a broader set of orientations.");
        }
        return false;
    }

    const Matrix3 quadratic {
        std::array<double, 3> {coefficients[0], coefficients[3], coefficients[4]},
        std::array<double, 3> {coefficients[3], coefficients[1], coefficients[5]},
        std::array<double, 3> {coefficients[4], coefficients[5], coefficients[2]}
    };
    const Vector3 linear {coefficients[6], coefficients[7], coefficients[8]};

    Matrix3 quadraticInverse {};
    if (!invertMatrix3(quadratic, &quadraticInverse)) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit cannot determine hard-iron offsets.");
        }
        return false;
    }

    Vector3 center = multiply(quadraticInverse, linear);
    for (double &value : center) {
        value = -value;
    }

    const double level = 1.0 + dot(center, multiply(quadratic, center));
    if (!std::isfinite(level) || level <= 0.0) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit is not physically valid.");
        }
        return false;
    }

    Matrix3 ellipsoid {};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            ellipsoid[row][column] = quadratic[row][column] / level;
        }
    }

    Vector3 eigenvalues {};
    Matrix3 eigenvectors {};
    if (!symmetricEigenDecomposition(ellipsoid, &eigenvalues, &eigenvectors)) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit eigenvalue calculation failed.");
        }
        return false;
    }

    for (const double eigenvalue : eigenvalues) {
        if (!std::isfinite(eigenvalue) || eigenvalue <= 1.0e-12) {
            if (error) {
                *error = QStringLiteral("Ellipsoid fit is not positive definite. Collect a broader set of orientations.");
            }
            return false;
        }
    }

    const double ellipsoidDeterminant = determinant(ellipsoid);
    if (!std::isfinite(ellipsoidDeterminant) || ellipsoidDeterminant <= 0.0) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit determinant is invalid.");
        }
        return false;
    }

    // sqrt(M) maps the fitted ellipsoid to a unit sphere. Scale it either to
    // the requested field norm or to the ellipsoid's geometric mean radius.
    const double autoTargetNormalized = std::pow(ellipsoidDeterminant, -1.0 / 6.0);
    const double appliedTarget = requestedFieldStrength > 0.0
        ? requestedFieldStrength
        : inputScale * autoTargetNormalized;
    const double targetNormalized = appliedTarget / inputScale;
    Matrix3 correction = symmetricSquareRoot(eigenvalues, eigenvectors);
    for (auto &row : correction) {
        for (double &value : row) {
            value *= targetNormalized;
        }
    }

    CalibrationResult fitted;
    for (int row = 0; row < 3; ++row) {
        fitted.hardIronOffsets[row] = mean[row] + inputScale * center[row];
        for (int column = 0; column < 3; ++column) {
            fitted.softIronMatrix[row * 3 + column] = correction[row][column];
        }
    }
    fitted.targetFieldStrength = appliedTarget;

    double squaredResidual = 0.0;
    for (const Sample &sample : samples) {
        Vector3 centered {
            sample.x - fitted.hardIronOffsets[0],
            sample.y - fitted.hardIronOffsets[1],
            sample.z - fitted.hardIronOffsets[2]
        };
        const Vector3 calibrated = multiply(correction, centered);
        const double magnitude = std::sqrt(dot(calibrated, calibrated));
        const double residual = (magnitude - appliedTarget) / appliedTarget;
        squaredResidual += residual * residual;
    }
    fitted.residualRmsPercent = 100.0 * std::sqrt(squaredResidual / samples.size());
    if (!std::isfinite(fitted.residualRmsPercent)) {
        if (error) {
            *error = QStringLiteral("Ellipsoid fit residual is invalid.");
        }
        return false;
    }

    *result = fitted;
    if (error) {
        error->clear();
    }
    return true;
}

void MagnetometerCalibration::processFrames(const QVector<_un_anotc_v8_frame> &frames)
{
    if (!m_collecting) {
        return;
    }

    for (const _un_anotc_v8_frame &frame : frames) {
        if (frame.frame.fun != ANOTC_FRAME_MAG || frame.frame.len < 6) {
            continue;
        }

        Sample sample;
        sample.x = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data));
        sample.y = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data + 2));
        sample.z = static_cast<double>(qFromLittleEndian<qint16>(frame.frame.data + 4));
        appendSample(sample);
        if (!m_collecting) {
            break;
        }
    }
}

QVariantList MagnetometerCalibration::pointCloud(bool calibrated) const
{
    QVariantList points;
    if (m_samples.isEmpty() || (calibrated && !m_calibrationValid)) {
        return points;
    }

    const int step = std::max(1,
                              static_cast<int>(std::ceil(static_cast<double>(m_samples.size())
                                                         / MaximumRenderedPointCount)));
    points.reserve((m_samples.size() + step - 1) / step);
    for (int index = 0; index < m_samples.size(); index += step) {
        const Sample &sample = m_samples.at(index);
        double x = sample.x;
        double y = sample.y;
        double z = sample.z;
        if (calibrated) {
            const double centeredX = x - m_result.hardIronOffsets[0];
            const double centeredY = y - m_result.hardIronOffsets[1];
            const double centeredZ = z - m_result.hardIronOffsets[2];
            x = m_result.softIronMatrix[0] * centeredX
                + m_result.softIronMatrix[1] * centeredY
                + m_result.softIronMatrix[2] * centeredZ;
            y = m_result.softIronMatrix[3] * centeredX
                + m_result.softIronMatrix[4] * centeredY
                + m_result.softIronMatrix[5] * centeredZ;
            z = m_result.softIronMatrix[6] * centeredX
                + m_result.softIronMatrix[7] * centeredY
                + m_result.softIronMatrix[8] * centeredZ;
        }

        points.append(QVariantMap {
            {QStringLiteral("x"), x},
            {QStringLiteral("y"), y},
            {QStringLiteral("z"), z}
        });
    }
    return points;
}

void MagnetometerCalibration::appendSample(const Sample &sample)
{
    if (m_hasLastAcceptedSample && distanceSquared(sample, m_lastAcceptedSample) < 1.0) {
        return;
    }

    m_samples.append(sample);
    m_lastAcceptedSample = sample;
    m_hasLastAcceptedSample = true;
    invalidateCalibration();
    emit sampleCountChanged();

    if (m_samples.size() == 1 || m_samples.size() % 5 == 0) {
        updateCoverage();
    }
    if (m_samples.size() == 1 || m_samples.size() % 10 == 0) {
        emit pointCloudChanged();
    }
    if (m_samples.size() >= MaximumSampleCount) {
        setCollecting(false);
        setLastError(QStringLiteral("Sample limit reached. Calculate the calibration or start a new collection."));
    }
}

void MagnetometerCalibration::updateCoverage()
{
    if (m_samples.isEmpty()) {
        setCoveragePercent(0);
        return;
    }

    Vector3 minimum {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };
    Vector3 maximum {
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()
    };
    for (const Sample &sample : m_samples) {
        const Vector3 value = toVector(sample);
        for (int axis = 0; axis < 3; ++axis) {
            minimum[axis] = std::min(minimum[axis], value[axis]);
            maximum[axis] = std::max(maximum[axis], value[axis]);
        }
    }

    Vector3 center {};
    Vector3 spans {};
    for (int axis = 0; axis < 3; ++axis) {
        center[axis] = 0.5 * (minimum[axis] + maximum[axis]);
        spans[axis] = maximum[axis] - minimum[axis];
    }
    const double largestSpan = *std::max_element(spans.begin(), spans.end());
    const double smallestSpan = *std::min_element(spans.begin(), spans.end());
    const double spanCoverage = largestSpan > 0.0
        ? std::min(1.0, smallestSpan / (largestSpan * 0.55))
        : 0.0;

    std::array<bool, 32> occupied {};
    constexpr double Pi = 3.14159265358979323846;
    for (const Sample &sample : m_samples) {
        const double x = sample.x - center[0];
        const double y = sample.y - center[1];
        const double z = sample.z - center[2];
        const double magnitude = std::sqrt(x * x + y * y + z * z);
        if (magnitude <= std::numeric_limits<double>::epsilon()) {
            continue;
        }

        double azimuth = std::atan2(y, x);
        if (azimuth < 0.0) {
            azimuth += 2.0 * Pi;
        }
        const int azimuthBucket = std::min(7, static_cast<int>(azimuth * 8.0 / (2.0 * Pi)));
        const double normalizedZ = std::clamp(z / magnitude, -1.0, 1.0);
        const int elevationBucket = std::min(3, static_cast<int>((normalizedZ + 1.0) * 2.0));
        occupied[elevationBucket * 8 + azimuthBucket] = true;
    }

    const int occupiedBucketCount = std::count(occupied.begin(), occupied.end(), true);
    const double sampleCoverage = std::min(1.0, m_samples.size() / 150.0);
    const double bucketCoverage = std::min(1.0, occupiedBucketCount / 24.0);
    setCoveragePercent(static_cast<int>(std::lround(100.0
                                                   * std::min({spanCoverage,
                                                               sampleCoverage,
                                                               bucketCoverage}))));
}

void MagnetometerCalibration::invalidateCalibration()
{
    if (!m_calibrationValid) {
        return;
    }

    m_calibrationValid = false;
    m_result = {};
    emit calibrationChanged();
    emit pointCloudChanged();
}

void MagnetometerCalibration::setCollecting(bool collecting)
{
    if (m_collecting == collecting) {
        return;
    }

    m_collecting = collecting;
    emit collectingChanged();
}

void MagnetometerCalibration::setCoveragePercent(int coveragePercent)
{
    coveragePercent = std::clamp(coveragePercent, 0, 100);
    if (m_coveragePercent == coveragePercent) {
        return;
    }

    m_coveragePercent = coveragePercent;
    emit coveragePercentChanged();
}

void MagnetometerCalibration::setLastError(const QString &lastError)
{
    if (m_lastError == lastError) {
        return;
    }

    m_lastError = lastError;
    emit lastErrorChanged();
}
