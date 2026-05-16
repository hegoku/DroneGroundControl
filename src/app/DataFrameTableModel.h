#pragma once

#include "../protocol/AnotcFrame.h"

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QVector>

class DataFrameTableModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ValueRole,
        TypeRole,
        DescriptionRole,
        DepthRole,
        ExpandedRole,
        ExpandableRole,
        FunctionRole,
        ParameterIndexRole
    };

    explicit DataFrameTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;

    Q_INVOKABLE void toggleExpanded(int row);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();
    Q_INVOKABLE void addToDataChart(int row);

public slots:
    void processFrames(const QVector<_un_anotc_v8_frame> &frames);

signals:
    void countChanged();
    void addSelectedData(int function, int parameterIndex);

private:
    enum class ParamType {
        UInt8,
        Int8,
        UInt16,
        Int16,
        UInt32,
        Int32,
        Float,
        Unsupported
    };

    struct Parameter
    {
        QString name;
        QString typeName;
        QString description;
        QString value;
        ParamType type = ParamType::Unsupported;
        int byteSize = 0;
    };

    struct FrameDefinition
    {
        quint8 function = 0;
        QString idText;
        QString name;
        QString description;
        QString frequencyValue;
        QVector<Parameter> parameters;
        quint64 counter = 0;
        quint64 previousCounter = 0;
        bool expanded = false;
    };

    struct VisibleRow
    {
        int definitionIndex = -1;
        int parameterIndex = -1;
    };

    bool loadDefinitions(const QString &path);
    void rebuildVisibleRows();
    void calculateFrequencies();
    void markValueDirty(int row);
    void flushValueChanges();
    QString readValue(const quint8 *payload, qsizetype payloadLength, int *offset, const Parameter &parameter) const;
    static ParamType parseParamType(const QString &typeName);
    static int byteSize(ParamType type);
    static quint64 parameterKey(quint8 function, int parameterIndex);

    QVector<FrameDefinition> m_definitions;
    QVector<VisibleRow> m_visibleRows;
    QHash<int, int> m_definitionByFunction;
    QHash<int, int> m_parentRowByFunction;
    QHash<quint64, int> m_parameterRowByKey;
    QSet<int> m_dirtyValueRows;
    QTimer m_frequencyTimer;
    QTimer m_flushTimer;
};
