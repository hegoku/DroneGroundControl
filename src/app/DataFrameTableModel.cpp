#include "DataFrameTableModel.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtEndian>

#include <algorithm>
#include <cstring>

DataFrameTableModel::DataFrameTableModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_flushTimer.setInterval(33);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &DataFrameTableModel::flushValueChanges);

    m_frequencyTimer.setInterval(1000);
    connect(&m_frequencyTimer, &QTimer::timeout, this, &DataFrameTableModel::calculateFrequencies);

    loadDefinitions(QStringLiteral(":/resources/DataFrameDefination.json"));
    rebuildVisibleRows();
    m_frequencyTimer.start();
}

int DataFrameTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleRows.size();
}

QVariant DataFrameTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleRows.size()) {
        return {};
    }

    const VisibleRow &row = m_visibleRows.at(index.row());
    if (row.definitionIndex < 0 || row.definitionIndex >= m_definitions.size()) {
        return {};
    }

    const FrameDefinition &definition = m_definitions.at(row.definitionIndex);
    const bool isParent = row.parameterIndex < 0;

    if (role == IdRole) {
        return isParent ? definition.idText : QString();
    }
    if (role == NameRole) {
        return isParent ? definition.name : definition.parameters.at(row.parameterIndex).name;
    }
    if (role == ValueRole) {
        return isParent ? definition.frequencyValue : definition.parameters.at(row.parameterIndex).value;
    }
    if (role == TypeRole) {
        return isParent ? QStringLiteral("Hz") : definition.parameters.at(row.parameterIndex).typeName;
    }
    if (role == DescriptionRole) {
        return isParent ? definition.description : definition.parameters.at(row.parameterIndex).description;
    }
    if (role == DepthRole) {
        return isParent ? 0 : 1;
    }
    if (role == ExpandedRole) {
        return isParent && definition.expanded;
    }
    if (role == ExpandableRole) {
        return isParent && !definition.parameters.isEmpty();
    }
    if (role == FunctionRole) {
        return definition.function;
    }
    if (role == ParameterIndexRole) {
        return row.parameterIndex;
    }

    return {};
}

QHash<int, QByteArray> DataFrameTableModel::roleNames() const
{
    return {
        {IdRole, "idText"},
        {NameRole, "nameText"},
        {ValueRole, "valueText"},
        {TypeRole, "typeText"},
        {DescriptionRole, "descriptionText"},
        {DepthRole, "depth"},
        {ExpandedRole, "expanded"},
        {ExpandableRole, "expandable"},
        {FunctionRole, "functionId"},
        {ParameterIndexRole, "parameterIndex"}
    };
}

int DataFrameTableModel::count() const
{
    return m_visibleRows.size();
}

void DataFrameTableModel::toggleExpanded(int row)
{
    if (row < 0 || row >= m_visibleRows.size()) {
        return;
    }

    const VisibleRow visibleRow = m_visibleRows.at(row);
    if (visibleRow.parameterIndex >= 0) {
        return;
    }

    FrameDefinition &definition = m_definitions[visibleRow.definitionIndex];
    if (definition.parameters.isEmpty()) {
        return;
    }

    definition.expanded = !definition.expanded;
    beginResetModel();
    rebuildVisibleRows();
    endResetModel();
    emit countChanged();
}

void DataFrameTableModel::expandAll()
{
    bool changed = false;
    for (FrameDefinition &definition : m_definitions) {
        if (!definition.expanded && !definition.parameters.isEmpty()) {
            definition.expanded = true;
            changed = true;
        }
    }

    if (!changed) {
        return;
    }

    beginResetModel();
    rebuildVisibleRows();
    endResetModel();
    emit countChanged();
}

void DataFrameTableModel::collapseAll()
{
    bool changed = false;
    for (FrameDefinition &definition : m_definitions) {
        if (definition.expanded) {
            definition.expanded = false;
            changed = true;
        }
    }

    if (!changed) {
        return;
    }

    beginResetModel();
    rebuildVisibleRows();
    endResetModel();
    emit countChanged();
}

void DataFrameTableModel::addToDataChart(int row)
{
    if (row < 0 || row >= m_visibleRows.size()) {
        return;
    }

    const VisibleRow &visibleRow = m_visibleRows.at(row);
    const FrameDefinition &definition = m_definitions.at(visibleRow.definitionIndex);
    if (visibleRow.parameterIndex >= 0) {
        emit addSelectedData(definition.function, visibleRow.parameterIndex);
        return;
    }

    for (int parameterIndex = 0; parameterIndex < definition.parameters.size(); ++parameterIndex) {
        emit addSelectedData(definition.function, parameterIndex);
    }
}

void DataFrameTableModel::processFrames(const QVector<_un_anotc_v8_frame> &frames)
{
    for (const _un_anotc_v8_frame &frame : frames) {
        const auto definitionIt = m_definitionByFunction.constFind(frame.frame.fun);
        if (definitionIt == m_definitionByFunction.constEnd()) {
            continue;
        }

        FrameDefinition &definition = m_definitions[*definitionIt];
        ++definition.counter;

        const quint8 *payload = frame.frame.data;
        const qsizetype payloadLength = frame.frame.len;
        int offset = 0;

        for (int parameterIndex = 0; parameterIndex < definition.parameters.size(); ++parameterIndex) {
            Parameter &parameter = definition.parameters[parameterIndex];
            const QString value = readValue(payload, payloadLength, &offset, parameter);
            if (value.isNull()) {
                break;
            }
            if (parameter.value == value) {
                continue;
            }

            parameter.value = value;
            const auto rowIt = m_parameterRowByKey.constFind(parameterKey(definition.function, parameterIndex));
            if (rowIt != m_parameterRowByKey.constEnd()) {
                markValueDirty(*rowIt);
            }
        }
    }
}

bool DataFrameTableModel::loadDefinitions(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        return false;
    }

    QVector<FrameDefinition> definitions;
    const QJsonArray frames = document.array();
    definitions.reserve(frames.size());

    for (const QJsonValue &frameValue : frames) {
        const QJsonObject frameObject = frameValue.toObject();
        const int id = frameObject.value(QStringLiteral("id")).toInt(-1);
        if (id < 0 || id > 0xFF) {
            continue;
        }

        FrameDefinition definition;
        definition.function = static_cast<quint8>(id);
        definition.idText = QStringLiteral("0x%1").arg(id, 2, 16, QLatin1Char('0'));
        definition.name = frameObject.value(QStringLiteral("name")).toString();
        definition.description = frameObject.value(QStringLiteral("desc")).toString();

        const QJsonArray parameters = frameObject.value(QStringLiteral("params")).toArray();
        definition.parameters.reserve(parameters.size());
        for (const QJsonValue &parameterValue : parameters) {
            const QJsonObject parameterObject = parameterValue.toObject();
            Parameter parameter;
            parameter.name = parameterObject.value(QStringLiteral("name")).toString();
            parameter.typeName = parameterObject.value(QStringLiteral("type")).toString();
            parameter.description = parameterObject.value(QStringLiteral("desc")).toString();
            parameter.type = parseParamType(parameter.typeName);
            parameter.byteSize = byteSize(parameter.type);
            if (parameter.byteSize <= 0) {
                continue;
            }
            definition.parameters.append(parameter);
        }

        definitions.append(definition);
    }

    std::sort(definitions.begin(), definitions.end(), [](const FrameDefinition &left, const FrameDefinition &right) {
        return left.function < right.function;
    });

    m_definitions = std::move(definitions);
    m_definitionByFunction.clear();
    for (int index = 0; index < m_definitions.size(); ++index) {
        m_definitionByFunction.insert(m_definitions.at(index).function, index);
    }

    return true;
}

void DataFrameTableModel::rebuildVisibleRows()
{
    m_visibleRows.clear();
    m_parentRowByFunction.clear();
    m_parameterRowByKey.clear();

    int row = 0;
    for (int definitionIndex = 0; definitionIndex < m_definitions.size(); ++definitionIndex) {
        const FrameDefinition &definition = m_definitions.at(definitionIndex);
        m_visibleRows.append({definitionIndex, -1});
        m_parentRowByFunction.insert(definition.function, row++);

        if (!definition.expanded) {
            continue;
        }

        for (int parameterIndex = 0; parameterIndex < definition.parameters.size(); ++parameterIndex) {
            m_visibleRows.append({definitionIndex, parameterIndex});
            m_parameterRowByKey.insert(parameterKey(definition.function, parameterIndex), row++);
        }
    }
}

void DataFrameTableModel::calculateFrequencies()
{
    for (FrameDefinition &definition : m_definitions) {
        const quint64 delta = definition.counter - definition.previousCounter;
        definition.previousCounter = definition.counter;

        const QString frequency = QString::number(delta);
        if (definition.frequencyValue == frequency) {
            continue;
        }

        definition.frequencyValue = frequency;
        const auto rowIt = m_parentRowByFunction.constFind(definition.function);
        if (rowIt != m_parentRowByFunction.constEnd()) {
            markValueDirty(*rowIt);
        }
    }
}

void DataFrameTableModel::markValueDirty(int row)
{
    if (row < 0 || row >= m_visibleRows.size()) {
        return;
    }

    m_dirtyValueRows.insert(row);
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

void DataFrameTableModel::flushValueChanges()
{
    if (m_dirtyValueRows.isEmpty()) {
        return;
    }

    QList<int> rows = m_dirtyValueRows.values();
    m_dirtyValueRows.clear();
    std::sort(rows.begin(), rows.end());

    int rangeStart = rows.first();
    int rangeEnd = rangeStart;
    for (int i = 1; i < rows.size(); ++i) {
        if (rows.at(i) == rangeEnd + 1) {
            rangeEnd = rows.at(i);
            continue;
        }

        emit dataChanged(index(rangeStart, 0), index(rangeEnd, 0), {ValueRole});
        rangeStart = rows.at(i);
        rangeEnd = rangeStart;
    }

    emit dataChanged(index(rangeStart, 0), index(rangeEnd, 0), {ValueRole});
}

QString DataFrameTableModel::readValue(const quint8 *payload,
                                  qsizetype payloadLength,
                                  int *offset,
                                  const Parameter &parameter) const
{
    if (parameter.byteSize <= 0 || *offset + parameter.byteSize > payloadLength) {
        return {};
    }

    const quint8 *value = payload + *offset;
    *offset += parameter.byteSize;

    switch (parameter.type) {
    case ParamType::UInt8:
        return QString::number(*value);
    case ParamType::Int8:
        return QString::number(static_cast<qint8>(*value));
    case ParamType::UInt16:
        return QString::number(qFromLittleEndian<quint16>(value));
    case ParamType::Int16:
        return QString::number(static_cast<qint16>(qFromLittleEndian<quint16>(value)));
    case ParamType::UInt32:
        return QString::number(qFromLittleEndian<quint32>(value));
    case ParamType::Int32:
        return QString::number(static_cast<qint32>(qFromLittleEndian<quint32>(value)));
    case ParamType::Float: {
        const quint32 raw = qFromLittleEndian<quint32>(value);
        float floatValue = 0.0f;
        std::memcpy(&floatValue, &raw, sizeof(floatValue));
        return QString::number(floatValue, 'g', 7);
    }
    case ParamType::Unsupported:
        return {};
    }

    return {};
}

DataFrameTableModel::ParamType DataFrameTableModel::parseParamType(const QString &typeName)
{
    const QString normalized = typeName.trimmed().toLower();
    if (normalized == QStringLiteral("uint8")) {
        return ParamType::UInt8;
    }
    if (normalized == QStringLiteral("int8")) {
        return ParamType::Int8;
    }
    if (normalized == QStringLiteral("uint16")) {
        return ParamType::UInt16;
    }
    if (normalized == QStringLiteral("int16")) {
        return ParamType::Int16;
    }
    if (normalized == QStringLiteral("uint32")) {
        return ParamType::UInt32;
    }
    if (normalized == QStringLiteral("int32")) {
        return ParamType::Int32;
    }
    if (normalized == QStringLiteral("float") || normalized == QStringLiteral("float32")) {
        return ParamType::Float;
    }

    return ParamType::Unsupported;
}

int DataFrameTableModel::byteSize(ParamType type)
{
    switch (type) {
    case ParamType::UInt8:
    case ParamType::Int8:
        return 1;
    case ParamType::UInt16:
    case ParamType::Int16:
        return 2;
    case ParamType::UInt32:
    case ParamType::Int32:
    case ParamType::Float:
        return 4;
    case ParamType::Unsupported:
        return 0;
    }

    return 0;
}

quint64 DataFrameTableModel::parameterKey(quint8 function, int parameterIndex)
{
    return (static_cast<quint64>(function) << 32) | static_cast<quint32>(parameterIndex);
}
