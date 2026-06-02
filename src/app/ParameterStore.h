#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>

class ConnectionSession;

class ParameterStore : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int dirtyCount READ dirtyCount NOTIFY dirtyCountChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        TypeNameRole,
        ValueRole,
        ValueHexRole,
        HasDefinitionRole,
        HasValueRole,
        DirtyRole,
        BusyRole,
        EditableRole,
        BusyReasonRole,
        OwnerRole,
        LastErrorRole,
        RevisionRole,
        DescriptionRole
    };
    Q_ENUM(Role)

    explicit ParameterStore(QObject *parent = nullptr);

    void setConnectionSession(ConnectionSession *connectionSession);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap parameter(int parameterId) const;
    Q_INVOKABLE bool isBusy(int parameterId) const;
    Q_INVOKABLE bool isEditable(int parameterId) const;
    Q_INVOKABLE int dirtyCount() const;
    Q_INVOKABLE QVariantList dirtyParameterIds() const;
    Q_INVOKABLE bool exportJson(const QUrl &fileUrl);
    Q_INVOKABLE bool setParameterValueText(int parameterId,
                                           const QString &text,
                                           const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE quint64 refreshParameterValue(int parameterId,
                                              const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE quint64 refreshParameterValue(int parameterId,
                                              const QJSValue &onSuccess,
                                              const QJSValue &onFailure,
                                              const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE quint64 refreshParameterInfo(int parameterId,
                                             const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE quint64 refreshParameterInfo(int parameterId,
                                             const QJSValue &onSuccess,
                                             const QJSValue &onFailure,
                                             const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE quint64 writeParameter(int parameterId,
                                       const QJSValue &onSuccess,
                                       const QJSValue &onFailure,
                                       const QString &owner = QStringLiteral("ParameterStore"));
    Q_INVOKABLE void clearError(int parameterId);

signals:
    void parameterChanged(int parameterId);
    void parameterBusyChanged(int parameterId, bool busy);
    void parameterWriteSucceeded(int parameterId);
    void parameterWriteFailed(int parameterId, QString reason);
    void dirtyCountChanged();
    void errorOccurred(QString message);

private:
    struct Entry
    {
        quint16 id = 0;
        QString name;
        int type = -1;
        QString description;
        QByteArray valueBytes;
        QByteArray draftBytes;
        bool hasDefinition = false;
        bool hasValue = false;
        bool dirty = false;
        bool busy = false;
        QString busyReason;
        QString owner;
        QString lastError;
        quint64 revision = 0;
        quint64 activeRequestId = 0;
    };

    Entry &ensureEntry(quint16 parameterId);
    const Entry *entryFor(quint16 parameterId) const;
    int rowFor(quint16 parameterId) const;
    QModelIndex indexFor(quint16 parameterId) const;
    QVariantMap entryToMap(const Entry &entry) const;
    void notifyEntryChanged(quint16 parameterId);
    void setEntryBusy(Entry *entry,
                      bool busy,
                      const QString &reason = QString(),
                      const QString &owner = QString(),
                      quint64 requestId = 0);
    void applyParameterInfo(const QVariantMap &frame);
    void applyParameterValue(const QVariantMap &frame);
    void invokeSuccessCallback(const QJSValue &callback, const QVariantMap &frame);
    void invokeFailureCallback(const QJSValue &callback, const QString &reason);
    quint64 writeParameterBytes(quint16 parameterId,
                                const QByteArray &value,
                                const QJSValue &onSuccess,
                                const QJSValue &onFailure,
                                const QString &owner);
    static QVariant decodeValue(const QByteArray &bytes, int type);
    static QString formatValueText(const QByteArray &bytes, int type);
    static bool encodeValueText(const QString &text, int type, QByteArray *bytes, QString *error);
    static QString typeName(int type);
    static QByteArray bytesFromHex(const QString &hex, bool *ok);
    static void appendUnsigned(QByteArray *bytes, quint64 value, int byteCount);
    static void appendSigned(QByteArray *bytes, qint64 value, int byteCount);

    ConnectionSession *m_connectionSession = nullptr;
    QVector<Entry> m_entries;
    QHash<quint16, int> m_rowById;
};
