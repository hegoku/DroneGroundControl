#include "ParameterStore.h"

#include "ConnectionSession.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSaveFile>

#include <cstring>
#include <limits>

ParameterStore::ParameterStore(QObject *parent)
    : QAbstractListModel(parent)
{
}

void ParameterStore::setConnectionSession(ConnectionSession *connectionSession)
{
    m_connectionSession = connectionSession;
}

int ParameterStore::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant ParameterStore::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case IdRole:
        return entry.id;
    case NameRole:
        return entry.name;
    case TypeRole:
        return entry.type;
    case TypeNameRole:
        return typeName(entry.type);
    case ValueRole:
        return decodeValue(entry.dirty ? entry.draftBytes : entry.valueBytes, entry.type);
    case ValueHexRole:
        return QString::fromLatin1((entry.dirty ? entry.draftBytes : entry.valueBytes).toHex(' ').toUpper());
    case HasDefinitionRole:
        return entry.hasDefinition;
    case HasValueRole:
        return entry.hasValue;
    case DirtyRole:
        return entry.dirty;
    case BusyRole:
        return entry.busy;
    case EditableRole:
        return !entry.busy;
    case BusyReasonRole:
        return entry.busyReason;
    case OwnerRole:
        return entry.owner;
    case LastErrorRole:
        return entry.lastError;
    case RevisionRole:
        return QVariant::fromValue(entry.revision);
    case DescriptionRole:
        return entry.description;
    default:
        return {};
    }
}

QHash<int, QByteArray> ParameterStore::roleNames() const
{
    return {
        {IdRole, "parameterId"},
        {NameRole, "name"},
        {TypeRole, "type"},
        {TypeNameRole, "typeName"},
        {ValueRole, "value"},
        {ValueHexRole, "valueHex"},
        {HasDefinitionRole, "hasDefinition"},
        {HasValueRole, "hasValue"},
        {DirtyRole, "dirty"},
        {BusyRole, "busy"},
        {EditableRole, "editable"},
        {BusyReasonRole, "busyReason"},
        {OwnerRole, "owner"},
        {LastErrorRole, "lastError"},
        {RevisionRole, "revision"},
        {DescriptionRole, "description"}
    };
}

QVariantMap ParameterStore::parameter(int parameterId) const
{
    if (parameterId < 0 || parameterId > 0xFFFF) {
        return {};
    }

    const Entry *entry = entryFor(static_cast<quint16>(parameterId));
    return entry ? entryToMap(*entry) : QVariantMap {};
}

bool ParameterStore::isBusy(int parameterId) const
{
    if (parameterId < 0 || parameterId > 0xFFFF) {
        return false;
    }

    const Entry *entry = entryFor(static_cast<quint16>(parameterId));
    return entry && entry->busy;
}

bool ParameterStore::isEditable(int parameterId) const
{
    return !isBusy(parameterId);
}

int ParameterStore::dirtyCount() const
{
    int count = 0;
    for (const Entry &entry : m_entries) {
        if (entry.dirty) {
            ++count;
        }
    }
    return count;
}

QVariantList ParameterStore::dirtyParameterIds() const
{
    QVariantList ids;
    for (const Entry &entry : m_entries) {
        if (entry.dirty) {
            ids.append(entry.id);
        }
    }
    return ids;
}

bool ParameterStore::exportJson(const QUrl &fileUrl)
{
    QString path = fileUrl.toLocalFile();
    if (path.isEmpty()) {
        path = fileUrl.toString();
    }
    if (path.isEmpty()) {
        emit errorOccurred(QStringLiteral("Parameter export path is empty."));
        return false;
    }
    if (QFileInfo(path).suffix().isEmpty()) {
        path.append(QStringLiteral(".json"));
    }

    QJsonArray parameters;
    for (const Entry &entry : m_entries) {
        if (!entry.hasDefinition && !entry.hasValue) {
            continue;
        }

        const QByteArray bytes = entry.dirty ? entry.draftBytes : entry.valueBytes;
        QJsonObject parameter;
        parameter.insert(QStringLiteral("id"), entry.id);
        parameter.insert(QStringLiteral("name"), entry.name);
        parameter.insert(QStringLiteral("type"), typeName(entry.type));
        parameter.insert(QStringLiteral("description"), entry.description);
        if (entry.hasValue || entry.dirty) {
            parameter.insert(QStringLiteral("value"), formatValueText(bytes, entry.type));
        }
        parameters.append(parameter);
    }

    if (parameters.isEmpty()) {
        emit errorOccurred(QStringLiteral("There are no loaded parameters to export."));
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("formatVersion"), 1);
    root.insert(QStringLiteral("exportedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("parameters"), parameters);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QStringLiteral("Failed to open %1 for writing.").arg(path));
        return false;
    }
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size() || !file.commit()) {
        emit errorOccurred(QStringLiteral("Failed to write parameter export to %1.").arg(path));
        return false;
    }
    return true;
}

bool ParameterStore::setParameterValueText(int parameterId, const QString &text, const QString &owner)
{
    if (parameterId < 0 || parameterId > 0xFFFF) {
        emit errorOccurred(QStringLiteral("Parameter id is out of range."));
        return false;
    }

    Entry &entry = ensureEntry(static_cast<quint16>(parameterId));
    if (entry.busy) {
        entry.lastError = QStringLiteral("Parameter %1 is busy.").arg(parameterId);
        notifyEntryChanged(entry.id);
        return false;
    }
    if (!entry.hasDefinition) {
        entry.lastError = QStringLiteral("Parameter %1 has no definition.").arg(parameterId);
        notifyEntryChanged(entry.id);
        return false;
    }

    QByteArray bytes;
    QString error;
    if (!encodeValueText(text, entry.type, &bytes, &error)) {
        entry.lastError = error;
        ++entry.revision;
        notifyEntryChanged(entry.id);
        return false;
    }

    const bool wasDirty = entry.dirty;
    entry.draftBytes = bytes;
    entry.dirty = entry.valueBytes != bytes || !entry.hasValue;
    entry.lastError.clear();
    entry.owner = entry.dirty ? owner : QString();
    ++entry.revision;
    notifyEntryChanged(entry.id);
    if (wasDirty != entry.dirty) {
        emit dirtyCountChanged();
    }
    return true;
}

quint64 ParameterStore::refreshParameterValue(int parameterId, const QString &owner)
{
    return refreshParameterValue(parameterId, QJSValue(), QJSValue(), owner);
}

quint64 ParameterStore::refreshParameterValue(int parameterId,
                                              const QJSValue &onSuccess,
                                              const QJSValue &onFailure,
                                              const QString &owner)
{
    if (!m_connectionSession || parameterId < 0 || parameterId > 0xFFFF) {
        const QString message = QStringLiteral("Invalid parameter value refresh request.");
        emit errorOccurred(message);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    Entry &entry = ensureEntry(static_cast<quint16>(parameterId));
    if (entry.busy) {
        const QString message = QStringLiteral("Parameter %1 is busy.").arg(parameterId);
        entry.lastError = message;
        notifyEntryChanged(entry.id);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    setEntryBusy(&entry, true, QStringLiteral("reading"), owner);
    notifyEntryChanged(entry.id);

    const quint16 id = static_cast<quint16>(parameterId);
    const quint64 requestId = m_connectionSession->requestParameterValue(
        id,
        [this, onSuccess](const QVariantMap &frame) {
            applyParameterValue(frame);
            invokeSuccessCallback(onSuccess, frame);
        },
        [this, id, onFailure](const QString &reason) {
            Entry &entry = ensureEntry(id);
            entry.lastError = reason;
            setEntryBusy(&entry, false);
            ++entry.revision;
            notifyEntryChanged(id);
            invokeFailureCallback(onFailure, reason);
        });
    entry.activeRequestId = requestId;
    notifyEntryChanged(entry.id);
    return requestId;
}

quint64 ParameterStore::refreshParameterInfo(int parameterId, const QString &owner)
{
    return refreshParameterInfo(parameterId, QJSValue(), QJSValue(), owner);
}

quint64 ParameterStore::refreshParameterInfo(int parameterId,
                                             const QJSValue &onSuccess,
                                             const QJSValue &onFailure,
                                             const QString &owner)
{
    if (!m_connectionSession || parameterId < 0 || parameterId > 0xFFFF) {
        const QString message = QStringLiteral("Invalid parameter info refresh request.");
        emit errorOccurred(message);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    Entry &entry = ensureEntry(static_cast<quint16>(parameterId));
    if (entry.busy) {
        const QString message = QStringLiteral("Parameter %1 is busy.").arg(parameterId);
        entry.lastError = message;
        notifyEntryChanged(entry.id);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    setEntryBusy(&entry, true, QStringLiteral("reading"), owner);
    notifyEntryChanged(entry.id);

    const quint16 id = static_cast<quint16>(parameterId);
    const quint64 requestId = m_connectionSession->requestParameterInfo(
        id,
        [this, onSuccess](const QVariantMap &frame) {
            applyParameterInfo(frame);
            invokeSuccessCallback(onSuccess, frame);
        },
        [this, id, onFailure](const QString &reason) {
            Entry &entry = ensureEntry(id);
            entry.lastError = reason;
            setEntryBusy(&entry, false);
            ++entry.revision;
            notifyEntryChanged(id);
            invokeFailureCallback(onFailure, reason);
        });
    entry.activeRequestId = requestId;
    notifyEntryChanged(entry.id);
    return requestId;
}

quint64 ParameterStore::writeParameter(int parameterId,
                                       const QJSValue &onSuccess,
                                       const QJSValue &onFailure,
                                       const QString &owner)
{
    if (parameterId < 0 || parameterId > 0xFFFF) {
        const QString message = QStringLiteral("Invalid parameter write request.");
        emit errorOccurred(message);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    Entry &entry = ensureEntry(static_cast<quint16>(parameterId));
    const QByteArray value = entry.dirty ? entry.draftBytes : entry.valueBytes;
    if (value.isEmpty() && entry.type != 10) {
        const QString message = QStringLiteral("Parameter %1 has no value to write.").arg(parameterId);
        entry.lastError = message;
        notifyEntryChanged(entry.id);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    return writeParameterBytes(static_cast<quint16>(parameterId), value, onSuccess, onFailure, owner);
}

quint64 ParameterStore::writeParameterBytes(quint16 parameterId,
                                            const QByteArray &value,
                                            const QJSValue &onSuccess,
                                            const QJSValue &onFailure,
                                            const QString &owner)
{
    if (!m_connectionSession) {
        const QString message = QStringLiteral("Invalid parameter write request.");
        emit errorOccurred(message);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    Entry &entry = ensureEntry(parameterId);
    if (entry.busy) {
        const QString message = QStringLiteral("Parameter %1 is busy.").arg(parameterId);
        entry.lastError = message;
        notifyEntryChanged(entry.id);
        emit parameterWriteFailed(parameterId, message);
        emit errorOccurred(message);
        invokeFailureCallback(onFailure, message);
        return 0;
    }

    const bool wasDirty = entry.dirty;
    entry.draftBytes = value;
    entry.dirty = true;
    entry.lastError.clear();
    setEntryBusy(&entry, true, QStringLiteral("writing"), owner);
    notifyEntryChanged(entry.id);

    const quint64 requestId = m_connectionSession->writeParameterRaw(
        parameterId,
        value,
        [this, parameterId, value, onSuccess, onFailure](const QVariantMap &response) {
            Entry &entry = ensureEntry(parameterId);
            const int ackCode = response.value(QStringLiteral("ackCode"), 0).toInt();
            if (ackCode != 0) {
                const QString message = response.value(QStringLiteral("ackMessage")).toString();
                entry.lastError = message;
                entry.dirty = true;
                setEntryBusy(&entry, false);
                ++entry.revision;
                notifyEntryChanged(parameterId);
                emit parameterWriteFailed(parameterId, message);
                invokeFailureCallback(onFailure, message);
                return;
            }

            entry.valueBytes = value;
            entry.draftBytes.clear();
            entry.hasValue = true;
            entry.dirty = false;
            entry.lastError.clear();
            setEntryBusy(&entry, false);
            ++entry.revision;
            notifyEntryChanged(parameterId);
            emit dirtyCountChanged();
            emit parameterWriteSucceeded(parameterId);
            invokeSuccessCallback(onSuccess, response);
        },
        [this, parameterId, onFailure](const QString &reason) {
            Entry &entry = ensureEntry(parameterId);
            entry.lastError = reason;
            entry.dirty = true;
            setEntryBusy(&entry, false);
            ++entry.revision;
            notifyEntryChanged(parameterId);
            emit parameterWriteFailed(parameterId, reason);
            invokeFailureCallback(onFailure, reason);
        });
    entry.activeRequestId = requestId;
    notifyEntryChanged(entry.id);
    if (!wasDirty) {
        emit dirtyCountChanged();
    }
    return requestId;
}

void ParameterStore::clearError(int parameterId)
{
    if (parameterId < 0 || parameterId > 0xFFFF) {
        return;
    }

    Entry &entry = ensureEntry(static_cast<quint16>(parameterId));
    if (entry.lastError.isEmpty()) {
        return;
    }
    entry.lastError.clear();
    notifyEntryChanged(entry.id);
}

ParameterStore::Entry &ParameterStore::ensureEntry(quint16 parameterId)
{
    const auto existing = m_rowById.constFind(parameterId);
    if (existing != m_rowById.constEnd()) {
        return m_entries[*existing];
    }

    const int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    Entry entry;
    entry.id = parameterId;
    m_entries.append(entry);
    m_rowById.insert(parameterId, row);
    endInsertRows();
    return m_entries[row];
}

const ParameterStore::Entry *ParameterStore::entryFor(quint16 parameterId) const
{
    const auto row = m_rowById.constFind(parameterId);
    return row == m_rowById.constEnd() ? nullptr : &m_entries.at(*row);
}

int ParameterStore::rowFor(quint16 parameterId) const
{
    return m_rowById.value(parameterId, -1);
}

QModelIndex ParameterStore::indexFor(quint16 parameterId) const
{
    const int row = rowFor(parameterId);
    return row < 0 ? QModelIndex() : index(row, 0);
}

QVariantMap ParameterStore::entryToMap(const Entry &entry) const
{
    QVariantMap map;
    map.insert(QStringLiteral("parameterId"), entry.id);
    map.insert(QStringLiteral("name"), entry.name);
    map.insert(QStringLiteral("type"), entry.type);
    map.insert(QStringLiteral("typeName"), typeName(entry.type));
    map.insert(QStringLiteral("value"), decodeValue(entry.dirty ? entry.draftBytes : entry.valueBytes, entry.type));
    map.insert(QStringLiteral("valueHex"), QString::fromLatin1((entry.dirty ? entry.draftBytes : entry.valueBytes).toHex(' ').toUpper()));
    map.insert(QStringLiteral("hasDefinition"), entry.hasDefinition);
    map.insert(QStringLiteral("hasValue"), entry.hasValue);
    map.insert(QStringLiteral("dirty"), entry.dirty);
    map.insert(QStringLiteral("busy"), entry.busy);
    map.insert(QStringLiteral("editable"), !entry.busy);
    map.insert(QStringLiteral("busyReason"), entry.busyReason);
    map.insert(QStringLiteral("owner"), entry.owner);
    map.insert(QStringLiteral("lastError"), entry.lastError);
    map.insert(QStringLiteral("revision"), QVariant::fromValue(entry.revision));
    map.insert(QStringLiteral("description"), entry.description);
    return map;
}

void ParameterStore::notifyEntryChanged(quint16 parameterId)
{
    const QModelIndex modelIndex = indexFor(parameterId);
    if (!modelIndex.isValid()) {
        return;
    }

    emit dataChanged(modelIndex, modelIndex);
    emit parameterChanged(parameterId);
}

void ParameterStore::setEntryBusy(Entry *entry,
                                  bool busy,
                                  const QString &reason,
                                  const QString &owner,
                                  quint64 requestId)
{
    if (!entry) {
        return;
    }

    const bool changed = entry->busy != busy;
    entry->busy = busy;
    entry->busyReason = busy ? reason : QString();
    entry->owner = busy ? owner : QString();
    entry->activeRequestId = busy ? requestId : 0;
    if (changed) {
        emit parameterBusyChanged(entry->id, busy);
    }
}

void ParameterStore::applyParameterInfo(const QVariantMap &frame)
{
    const int idValue = frame.value(QStringLiteral("parameterId"), -1).toInt();
    if (idValue < 0 || idValue > 0xFFFF) {
        return;
    }

    const quint16 id = static_cast<quint16>(idValue);
    Entry &entry = ensureEntry(id);
    entry.type = frame.value(QStringLiteral("parameterType"), entry.type).toInt();
    entry.name = frame.value(QStringLiteral("parameterName"), entry.name).toString();
    entry.description = frame.value(QStringLiteral("parameterInfo"), entry.description).toString();
    entry.hasDefinition = true;
    entry.lastError.clear();
    setEntryBusy(&entry, false);
    ++entry.revision;
    notifyEntryChanged(id);
}

void ParameterStore::applyParameterValue(const QVariantMap &frame)
{
    const int idValue = frame.value(QStringLiteral("parameterId"), -1).toInt();
    if (idValue < 0 || idValue > 0xFFFF) {
        return;
    }

    const quint16 id = static_cast<quint16>(idValue);
    Entry &entry = ensureEntry(id);
    bool ok = false;
    const QByteArray bytes = bytesFromHex(frame.value(QStringLiteral("parameterValueHex")).toString(), &ok);
    if (!ok) {
        return;
    }

    const bool wasDirty = entry.dirty;
    entry.valueBytes = bytes;
    entry.draftBytes.clear();
    entry.hasValue = true;
    entry.dirty = false;
    entry.lastError.clear();
    setEntryBusy(&entry, false);
    ++entry.revision;
    notifyEntryChanged(id);
    if (wasDirty) {
        emit dirtyCountChanged();
    }
}

void ParameterStore::invokeSuccessCallback(const QJSValue &callback, const QVariantMap &frame)
{
    if (!callback.isCallable()) {
        return;
    }

    QJSEngine *engine = qjsEngine(this);
    if (!engine && m_connectionSession) {
        engine = qjsEngine(m_connectionSession);
    }
    if (!engine) {
        return;
    }

    QJSValue callable = callback;
    callable.call({ engine->toScriptValue(frame) });
}

void ParameterStore::invokeFailureCallback(const QJSValue &callback, const QString &reason)
{
    if (!callback.isCallable()) {
        return;
    }

    QJSEngine *engine = qjsEngine(this);
    if (!engine && m_connectionSession) {
        engine = qjsEngine(m_connectionSession);
    }
    if (!engine) {
        return;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("reason"), reason);

    QJSValue callable = callback;
    callable.call({ engine->toScriptValue(payload) });
}

QVariant ParameterStore::decodeValue(const QByteArray &bytes, int type)
{
    if (bytes.isEmpty()) {
        return {};
    }

    auto readUnsigned = [&bytes](int count) -> quint64 {
        quint64 value = 0;
        for (int i = 0; i < count && i < bytes.size(); ++i) {
            value |= static_cast<quint64>(static_cast<quint8>(bytes.at(i))) << (8 * i);
        }
        return value;
    };

    switch (type) {
    case 0:
        return static_cast<quint8>(readUnsigned(1));
    case 1:
        return static_cast<qint8>(readUnsigned(1));
    case 2:
        return static_cast<quint16>(readUnsigned(2));
    case 3:
        return static_cast<qint16>(readUnsigned(2));
    case 4:
        return static_cast<quint32>(readUnsigned(4));
    case 5:
        return static_cast<qint32>(readUnsigned(4));
    case 6:
        return QString::number(readUnsigned(8));
    case 7:
        return QString::number(static_cast<qint64>(readUnsigned(8)));
    case 8: {
        const quint32 raw = static_cast<quint32>(readUnsigned(4));
        float value = 0.0f;
        std::memcpy(&value, &raw, sizeof(float));
        return value;
    }
    case 9: {
        const quint64 raw = readUnsigned(8);
        double value = 0.0;
        std::memcpy(&value, &raw, sizeof(double));
        return value;
    }
    case 10:
        return QString::fromUtf8(bytes);
    default:
        return QString::fromLatin1(bytes.toHex(' ').toUpper());
    }
}

QString ParameterStore::formatValueText(const QByteArray &bytes, int type)
{
    const QVariant value = decodeValue(bytes, type);
    if (!value.isValid()) {
        return {};
    }

    switch (type) {
    case 8:
        return QString::number(value.toFloat(), 'g', 7);
    case 9:
        return QString::number(value.toDouble(), 'g', 15);
    default:
        return value.toString();
    }
}

bool ParameterStore::encodeValueText(const QString &text, int type, QByteArray *bytes, QString *error)
{
    if (!bytes) {
        return false;
    }

    auto fail = [error](const QString &message) {
        if (error) {
            *error = message;
        }
        return false;
    };

    bytes->clear();
    const QString trimmed = text.trimmed();
    bool ok = false;

    switch (type) {
    case 0: {
        const quint64 value = trimmed.toULongLong(&ok);
        if (!ok || value > std::numeric_limits<quint8>::max()) {
            return fail(QStringLiteral("Value must be UInt8."));
        }
        appendUnsigned(bytes, value, 1);
        return true;
    }
    case 1: {
        const qint64 value = trimmed.toLongLong(&ok);
        if (!ok || value < std::numeric_limits<qint8>::min() || value > std::numeric_limits<qint8>::max()) {
            return fail(QStringLiteral("Value must be Int8."));
        }
        appendSigned(bytes, value, 1);
        return true;
    }
    case 2: {
        const quint64 value = trimmed.toULongLong(&ok);
        if (!ok || value > std::numeric_limits<quint16>::max()) {
            return fail(QStringLiteral("Value must be UInt16."));
        }
        appendUnsigned(bytes, value, 2);
        return true;
    }
    case 3: {
        const qint64 value = trimmed.toLongLong(&ok);
        if (!ok || value < std::numeric_limits<qint16>::min() || value > std::numeric_limits<qint16>::max()) {
            return fail(QStringLiteral("Value must be Int16."));
        }
        appendSigned(bytes, value, 2);
        return true;
    }
    case 4: {
        const quint64 value = trimmed.toULongLong(&ok);
        if (!ok || value > std::numeric_limits<quint32>::max()) {
            return fail(QStringLiteral("Value must be UInt32."));
        }
        appendUnsigned(bytes, value, 4);
        return true;
    }
    case 5: {
        const qint64 value = trimmed.toLongLong(&ok);
        if (!ok || value < std::numeric_limits<qint32>::min() || value > std::numeric_limits<qint32>::max()) {
            return fail(QStringLiteral("Value must be Int32."));
        }
        appendSigned(bytes, value, 4);
        return true;
    }
    case 6: {
        const quint64 value = trimmed.toULongLong(&ok);
        if (!ok) {
            return fail(QStringLiteral("Value must be UInt64."));
        }
        appendUnsigned(bytes, value, 8);
        return true;
    }
    case 7: {
        const qint64 value = trimmed.toLongLong(&ok);
        if (!ok) {
            return fail(QStringLiteral("Value must be Int64."));
        }
        appendSigned(bytes, value, 8);
        return true;
    }
    case 8: {
        const float value = trimmed.toFloat(&ok);
        if (!ok) {
            return fail(QStringLiteral("Value must be Float."));
        }
        quint32 raw = 0;
        std::memcpy(&raw, &value, sizeof(float));
        appendUnsigned(bytes, raw, 4);
        return true;
    }
    case 9: {
        const double value = trimmed.toDouble(&ok);
        if (!ok) {
            return fail(QStringLiteral("Value must be Double."));
        }
        quint64 raw = 0;
        std::memcpy(&raw, &value, sizeof(double));
        appendUnsigned(bytes, raw, 8);
        return true;
    }
    case 10:
        bytes->append(text.toUtf8());
        return true;
    default:
        return fail(QStringLiteral("Parameter type is unknown."));
    }
}

QString ParameterStore::typeName(int type)
{
    switch (type) {
    case 0: return QStringLiteral("UInt8");
    case 1: return QStringLiteral("Int8");
    case 2: return QStringLiteral("UInt16");
    case 3: return QStringLiteral("Int16");
    case 4: return QStringLiteral("UInt32");
    case 5: return QStringLiteral("Int32");
    case 6: return QStringLiteral("UInt64");
    case 7: return QStringLiteral("Int64");
    case 8: return QStringLiteral("Float");
    case 9: return QStringLiteral("Double");
    case 10: return QStringLiteral("String");
    default: return QStringLiteral("Unknown");
    }
}

QByteArray ParameterStore::bytesFromHex(const QString &hex, bool *ok)
{
    QString normalized = hex;
    normalized.remove(QRegularExpression(QStringLiteral("\\s+")));

    static const QRegularExpression validHex(QStringLiteral("^[0-9a-fA-F]*$"));
    const bool valid = normalized.size() % 2 == 0 && validHex.match(normalized).hasMatch();
    if (ok) {
        *ok = valid;
    }
    return valid ? QByteArray::fromHex(normalized.toLatin1()) : QByteArray();
}

void ParameterStore::appendUnsigned(QByteArray *bytes, quint64 value, int byteCount)
{
    for (int i = 0; i < byteCount; ++i) {
        bytes->append(static_cast<char>((value >> (8 * i)) & 0xFF));
    }
}

void ParameterStore::appendSigned(QByteArray *bytes, qint64 value, int byteCount)
{
    appendUnsigned(bytes, static_cast<quint64>(value), byteCount);
}
