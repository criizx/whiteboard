#include <QJsonArray>
#include <QDebug>

#include <io/Network/NetworkTransport.h>
#include <io/Network/PeerTransport.h>

static constexpr int RECONNECT_INTERVAL_MS = 3000;

NetworkTransport::NetworkTransport(QObject* parent)
    : QObject(parent), m_peerTransport(new PeerTransport(this))
{
    connect(&m_socket, &QWebSocket::connected,    this, &NetworkTransport::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &NetworkTransport::onDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived,
            this, &NetworkTransport::onTextMessageReceived);
    connect(&m_socket, &QWebSocket::errorOccurred,
            this, &NetworkTransport::onError);

    m_reconnectTimer.setInterval(RECONNECT_INTERVAL_MS);
    m_reconnectTimer.setSingleShot(false);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &NetworkTransport::onReconnectTimer);
    connect(m_peerTransport, &PeerTransport::signalToSend, this, [this](const QJsonObject& message) {
        if (isConnected()) {
            m_socket.sendTextMessage(QString::fromUtf8(
                QJsonDocument(message).toJson(QJsonDocument::Compact)));
        }
    });
    connect(m_peerTransport, &PeerTransport::deltaReceived,
            this, &NetworkTransport::deltaReceived);
    connect(m_peerTransport, &PeerTransport::snapshotRequested,
            this, &NetworkTransport::snapshotRequested);
    connect(m_peerTransport, &PeerTransport::peerCountChanged,
            this, &NetworkTransport::peerCountChanged);
    connect(m_peerTransport, &PeerTransport::peerCountChanged, this, [](int count) {
        qDebug() << "[PeerTransport] Direct peers:" << count;
    });
}

void NetworkTransport::connectToServer(const QUrl& serverUrl,
                                       const QString& roomId,
                                       const QString& clientId)
{
    m_serverUrl  = serverUrl;
    m_roomId     = roomId;
    m_clientId   = clientId;
    m_intentionalDisconnect = false;
    m_peerTransport->reset();

    qDebug() << "[NetworkTransport] Connecting to" << serverUrl;
    m_socket.open(serverUrl);
}

void NetworkTransport::disconnectFromServer()
{
    m_intentionalDisconnect = true;
    m_reconnectTimer.stop();
    m_peerTransport->reset();
    m_socket.close();
}

bool NetworkTransport::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkTransport::sendDelta(const QJsonObject& delta)
{
    if (m_peerTransport->broadcastDelta(delta) > 0) return;

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

    } else if (type == "peers") {
        m_peerTransport->connectToPeers(frame["payload"].toArray());
    } else if (type == "peer-left") {
        m_peerTransport->removePeer(frame["clientId"].toString());
    } else if (type == "peer-joined") {
        return;
    } else if (type == "offer" || type == "answer" || type == "candidate") {
        m_peerTransport->handleSignal(frame);
    } else {
        qDebug() << "[NetworkTransport] Unhandled frame type:" << type;
    }
}

void NetworkTransport::sendSnapshot(const QString& peerId, const QJsonArray& snapshot)
{
    m_peerTransport->sendSnapshot(peerId, snapshot);
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
