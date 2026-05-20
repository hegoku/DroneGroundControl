#pragma once

#include "../protocol/AnotcFrame.h"

#include <QAbstractListModel>
#include <QVector>

class RcChannelModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool hasSignal READ hasSignal NOTIFY signalStateChanged)
    Q_PROPERTY(quint64 frameCount READ frameCount NOTIFY frameCountChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        ValueRole,
        ValueTextRole,
        NormalizedRole,
        ActiveRole
    };

    explicit RcChannelModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    bool hasSignal() const;
    quint64 frameCount() const;

public slots:
    void processFrames(const QVector<_un_anotc_v8_frame> &frames);
    void reset();

signals:
    void countChanged();
    void signalStateChanged();
    void frameCountChanged();

private:
    struct Channel
    {
        QString label;
        int payloadIndex = 0;
        int value = 0;
        bool active = false;
    };

    static int readLeInt16(const quint8 *data);
    static double normalizedValue(int value);

    QVector<Channel> m_channels;
    bool m_hasSignal = false;
    quint64 m_frameCount = 0;
};
