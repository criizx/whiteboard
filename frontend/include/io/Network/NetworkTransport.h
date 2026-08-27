#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QTimer>

#include <QtWebSockets/QWebSocket>

class PeerTransport;

class NetworkTransport : public QObject {
    Q_OBJECT

public:
    explicit NetworkTransport(QObject* parent = nullptr);

    void connectToServer(const QUrl& serverUrl, const QString& roomId, const QString& clientId);
    void disconnectFromServer();

    [[nodiscard]] bool isConnected() const;
    void sendSnapshot(const QString& peerId, const QJsonArray& snapshot);

public slots:
    void sendDelta(const QJsonObject& delta);

signals:
    void deltaReceived(const QJsonObject& delta);
    void snapshotRequested(const QString& peerId);
    void peerCountChanged(int count);

    void connected();
    void disconnected();
    void errorOccurred(const QString& errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    QWebSocket  m_socket;
    QTimer      m_reconnectTimer;
    PeerTransport* m_peerTransport;

    QUrl    m_serverUrl;
    QString m_roomId;
    QString m_clientId;

    bool m_intentionalDisconnect = false;

    void joinRoom();
};
