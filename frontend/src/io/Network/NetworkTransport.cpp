#include <QJsonArray>
#include <QDebug>

#include <io/Network/NetworkTransport.h>

static constexpr int RECONNECT_INTERVAL_MS = 3000;

NetworkTransport::NetworkTransport(QObject* parent)
    : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected,    this, &NetworkTransport::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &NetworkTransport::onDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &NetworkTransport::onTextMessageReceived);
    connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &NetworkTransport::onError);

    m_reconnectTimer.setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer.setSingleShot(false);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &NetworkTransport::onReconnectTimer);
}

void NetworkTransport::connectToServer(const QUrl& serverUrl,
                                       const QString& roomId,
                                       const QString& clientId)
{
    m_serverUrl  = serverUrl;
    m_roomId     = roomId;
    m_clientId   = clientId;
    m_intentionalDisconnect = false;

    qDebug() << "[NetworkTransport] Connecting to" << serverUrl;
    m_socket.open(serverUrl);
}

void NetworkTransport::disconnectFromServer()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer.stop();
    m_socket.close();
}

bool NetworkTransport::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkTransport::sendDelta(const QJsonObject& delta)
{
    if (!isConnected()) {
        qWarning() << "[NetworkTransport] Cannot send delta: not connected";
        return;
    }

    QJsonObject envelope;
    envelope["type"]     = "delta";
    envelope["room"]     = m_roomId;
    envelope["clientId"] = m_clientId;
    envelope["payload"]  = delta;

    const QByteArray json = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
    m_socket.sendTextMessage(QString::fromUtf8(json));
}

void NetworkTransport::onConnected()
{
    qDebug() << "[NetworkTransport] Connected";
    m_reconnectTimer.stop();
    joinRoom();
    emit connected();
}

void NetworkTransport::onDisconnected()
{
    qDebug() << "[NetworkTransport] Disconnected";
    emit disconnected();

    if (!m_intentionalDisconnect) {
        qDebug() << "[NetworkTransport] Scheduling reconnect in"
                 << RECONNECT_INTERVAL_MS << "ms";
        m_reconnectTimer.start();
    }
}

void NetworkTransport::onTextMessageReceived(const QString& message)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[NetworkTransport] JSON parse error:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[NetworkTransport] Received non-object JSON frame";
        return;
    }

    const QJsonObject frame = doc.object();
    const QString type = frame["type"].toString();

    if (type == "delta") {
        if (frame["clientId"].toString() == m_clientId) return;

        const QJsonObject payload = frame["payload"].toObject();
        if (!payload.isEmpty()) {
            emit deltaReceived(payload);
        }

    } else if (type == "snapshot") {
        const QJsonArray deltas = frame["payload"].toArray();
        for (const auto& val : deltas) {
            const QJsonObject delta = val.toObject();
            if (!delta.isEmpty()) {
                emit deltaReceived(delta);
            }
        }

    } else {
        qDebug() << "[NetworkTransport] Unhandled frame type:" << type;
    }
}

void NetworkTransport::onError(QAbstractSocket::SocketError /*error*/)
{
    const QString msg = m_socket.errorString();
    qWarning() << "[NetworkTransport] Socket error:" << msg;
    emit errorOccurred(msg);
}

void NetworkTransport::onReconnectTimer()
{
    if (isConnected()) {
        m_reconnectTimer.stop();
        return;
    }
    qDebug() << "[NetworkTransport] Attempting reconnect…";
    m_socket.open(m_serverUrl);
}

void NetworkTransport::joinRoom()
{
    QJsonObject join;
    join["type"]     = "join";
    join["room"]     = m_roomId;
    join["clientId"] = m_clientId;

    const QByteArray json = QJsonDocument(join).toJson(QJsonDocument::Compact);
    m_socket.sendTextMessage(QString::fromUtf8(json));
    qDebug() << "[NetworkTransport] Joined room" << m_roomId;
}
