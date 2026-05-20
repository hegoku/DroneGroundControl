#pragma once

#include "AnotcRequestManager.h"
#include "ParameterStore.h"

#include "../connection/ConnectionConfig.h"
#include "../connection/ITransport.h"
#include "../protocol/IProtocolSession.h"

#include <QByteArray>
#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <functional>
#include <memory>

class ConnectionSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorOccurred)
    Q_PROPERTY(bool rawByteForwardingEnabled READ rawByteForwardingEnabled WRITE setRawByteForwardingEnabled NOTIFY rawByteForwardingEnabledChanged)
    Q_PROPERTY(ParameterStore *parameterStore READ parameterStore CONSTANT)

public:
    using RequestSuccessHandler = std::function<void(const QVariantMap &response)>;
    using RequestFailureHandler = std::function<void(const QString &reason)>;

    explicit ConnectionSession(QObject *parent = nullptr);
    ~ConnectionSession() override;

    bool isOpen() const;
    QString state() const;
    QString lastError() const;
    bool rawByteForwardingEnabled() const;
    void setRawByteForwardingEnabled(bool enabled);
    ParameterStore *parameterStore();

    Q_INVOKABLE QStringList availableSerialPorts() const;
    Q_INVOKABLE bool openSerial(const QString &portName,
                                int baudRate,
                                int dataBits,
                                const QString &parity,
                                const QString &stopBit);
    Q_INVOKABLE bool openUdp(const QString &host, int remotePort, int localPort);
    Q_INVOKABLE void close();
    Q_INVOKABLE void closeWithDisconnectCommand();
    Q_INVOKABLE void sendBytes(const QByteArray &data);
    Q_INVOKABLE quint64 requestDeviceInfo(quint16 type,
                                          const QJSValue &onSuccess = QJSValue(),
                                          const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 requestParameterCount(const QJSValue &onSuccess = QJSValue(),
                                              const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 requestParameterValue(int parameterId,
                                              const QJSValue &onSuccess = QJSValue(),
                                              const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 requestParameterInfo(int parameterId,
                                             const QJSValue &onSuccess = QJSValue(),
                                             const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 saveParameters(const QJSValue &onSuccess = QJSValue(),
                                       const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 sendReliableParameterCommand(int command,
                                                     int value = 0,
                                                     const QJSValue &onSuccess = QJSValue(),
                                                     const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 sendAnotcCommand(int cid0,
                                         int cid1,
                                         int cid2,
                                         const QString &valuesHex = QString(),
                                         const QJSValue &onSuccess = QJSValue(),
                                         const QJSValue &onFailure = QJSValue());
    Q_INVOKABLE quint64 sendAnotcCommandSequence(const QVariantList &commands,
                                                 const QJSValue &onSuccess = QJSValue(),
                                                 const QJSValue &onFailure = QJSValue());

    quint64 requestParameterCount(RequestSuccessHandler onSuccess,
                                  RequestFailureHandler onFailure = {});
    quint64 requestParameterValue(int parameterId,
                                  RequestSuccessHandler onSuccess,
                                  RequestFailureHandler onFailure = {});
    quint64 requestParameterInfo(int parameterId,
                                 RequestSuccessHandler onSuccess,
                                 RequestFailureHandler onFailure = {});
    quint64 writeParameterRaw(int parameterId,
                              const QByteArray &value,
                              RequestSuccessHandler onSuccess,
                              RequestFailureHandler onFailure = {});

signals:
    void isOpenChanged();
    void stateChanged();
    void rawByteForwardingEnabledChanged();
    void errorOccurred(QString message);
    void bytesReceived(QByteArray data);
    void telemetryFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void parameterFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void ackFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void commandFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void logFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void unknownFramesReceived(QVector<_un_anotc_v8_frame> frames);
    void performanceStats(TransportPerformanceStats stats);
    void requestStarted(quint64 requestId, QString name);
    void requestSucceeded(quint64 requestId, QString name);
    void requestFailed(quint64 requestId, QString name, QString reason);
    void sequenceStarted(quint64 sequenceId, QString name);
    void sequenceSucceeded(quint64 sequenceId, QString name);
    void sequenceFailed(quint64 sequenceId, QString name, QString reason);
    void log(QString message);

private:
    struct JsCallbacks
    {
        QJSValue onSuccess;
        QJSValue onFailure;
    };

    struct CppCallbacks
    {
        RequestSuccessHandler onSuccess;
        RequestFailureHandler onFailure;
    };

    void setTransport(ITransport *transport);
    void setProtocol(std::unique_ptr<IProtocolSession> protocol);
    void resetConnectionObjects();
    void setOpen(bool open);
    void setStateString(const QString &state);
    void setLastError(const QString &message);
    quint64 submitRequest(const AnotcRequest &request);
    quint64 submitRequest(const AnotcRequest &request,
                          const QJSValue &onSuccess,
                          const QJSValue &onFailure);
    quint64 submitRequest(const AnotcRequest &request,
                          RequestSuccessHandler onSuccess,
                          RequestFailureHandler onFailure);
    void storeRequestCallbacks(quint64 requestId,
                               const QJSValue &onSuccess,
                               const QJSValue &onFailure);
    void storeSequenceCallbacks(quint64 sequenceId,
                                const QJSValue &onSuccess,
                                const QJSValue &onFailure);
    void invokeRequestSuccess(quint64 requestId, const QString &name, const _un_anotc_v8_frame &response);
    void invokeRequestFailure(quint64 requestId, const QString &name, const QString &reason);
    void invokeSequenceSuccess(quint64 sequenceId, const QString &name, const _un_anotc_v8_frame &response);
    void invokeSequenceFailure(quint64 sequenceId, const QString &name, const QString &reason);
    void invokeSuccessCallback(const QJSValue &callback,
                               quint64 id,
                               const QString &name,
                               const _un_anotc_v8_frame &response);
    void invokeSuccessCallback(const RequestSuccessHandler &callback,
                               quint64 id,
                               const QString &name,
                               const _un_anotc_v8_frame &response);
    void invokeFailureCallback(const QJSValue &callback,
                               quint64 id,
                               const QString &name,
                               const QString &reason);
    void invokeFailureCallback(const RequestFailureHandler &callback,
                               quint64 id,
                               const QString &name,
                               const QString &reason);
    QVariantMap frameToVariantMap(const _un_anotc_v8_frame &frame) const;
    static QByteArray bytesFromHex(const QString &hex, bool *ok);
    static bool isByteValue(int value);
    static bool isU16Value(int value);
    static QString transportStateToString(ITransport::State state);

    QThread m_ioThread;
    QPointer<ITransport> m_transport;
    std::unique_ptr<IProtocolSession> m_protocol;
    AnotcRequestManager m_requestManager;
    ParameterStore m_parameterStore;
    QHash<quint64, JsCallbacks> m_requestCallbacks;
    QHash<quint64, CppCallbacks> m_cppRequestCallbacks;
    QHash<quint64, JsCallbacks> m_sequenceCallbacks;
    bool m_isOpen = false;
    bool m_rawByteForwardingEnabled = false;
    QString m_state = QStringLiteral("closed");
    QString m_lastError;
};
