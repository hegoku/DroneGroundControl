#pragma once

#include "../protocol/AnotcResponseMatcher.h"

#include <QByteArray>
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVector>

struct AnotcRequest
{
    QString name;
    QByteArray encodedFrame;
    QVector<AnotcResponseMatcher> responseMatchers;
    int timeoutMs = 1000;
    int maxRetries = 3;
};

class AnotcRequestManager : public QObject
{
    Q_OBJECT

public:
    explicit AnotcRequestManager(QObject *parent = nullptr);

    quint64 submit(const AnotcRequest &request);
    quint64 submitSequence(const QString &name, const QVector<AnotcRequest> &steps);
    void cancelAll(const QString &reason);

    static AnotcRequest makeCommandRequest(quint8 cid0,
                                           quint8 cid1,
                                           quint8 cid2,
                                           const QByteArray &values = {});
    static AnotcRequest makeReadDeviceInfoRequest(quint16 type);
    static AnotcRequest makeReadParameterCountRequest();
    static AnotcRequest makeReadParameterValueRequest(quint16 parameterId);
    static AnotcRequest makeReadParameterInfoRequest(quint16 parameterId);
    static AnotcRequest makeWriteParameterRequest(quint16 parameterId, const QByteArray &value);
    static AnotcRequest makeReliableParameterCommandRequest(quint8 command, quint16 value = 0);

public slots:
    void handleFrames(const AnotcParseResult &result);

signals:
    void sendBytes(QByteArray data);
    void requestStarted(quint64 requestId, QString name);
    void requestSucceeded(quint64 requestId, QString name, _un_anotc_v8_frame response);
    void requestFailed(quint64 requestId, QString name, QString reason);
    void sequenceStarted(quint64 sequenceId, QString name);
    void sequenceSucceeded(quint64 sequenceId, QString name, _un_anotc_v8_frame response);
    void sequenceFailed(quint64 sequenceId, QString name, QString reason);
    void log(QString message);

private slots:
    void handleTimeout();

private:
    struct QueueItem
    {
        quint64 requestId = 0;
        quint64 sequenceId = 0;
        QString sequenceName;
        QVector<AnotcRequest> sequenceSteps;
        int sequenceStepIndex = 0;
        AnotcRequest request;
        int attempts = 0;
    };

    static QByteArray encodeFrame(quint8 function,
                                  const QByteArray &payload,
                                  quint8 sourceAddress = DefaultSourceAddress,
                                  quint8 destinationAddress = DefaultDestinationAddress);
    static void appendChecksum(QByteArray *frame);
    static void appendLe16(QByteArray *payload, quint16 value);

    void enqueue(const QueueItem &item);
    void startNext();
    void sendCurrentAttempt();
    void completeCurrent(const _un_anotc_v8_frame &response);
    void failCurrent(const QString &reason);

    QQueue<QueueItem> m_queue;
    bool m_hasActive = false;
    QueueItem m_active;
    QTimer m_timeoutTimer;
    quint64 m_nextRequestId = 1;
    quint64 m_nextSequenceId = 1;

    static constexpr quint8 DefaultSourceAddress = 0xFE;
    static constexpr quint8 DefaultDestinationAddress = 0x01;
};
