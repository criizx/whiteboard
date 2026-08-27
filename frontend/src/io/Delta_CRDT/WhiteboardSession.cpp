#include <io/delta_CRDT/WhiteboardSession.h>
#include <Shared/Shared.h>
#include <QJsonArray>

WhiteboardSession::WhiteboardSession(QObject* parent)
    : QObject(parent)
{
    connect(&m_crdt, &DeltaCRDT::objectsUpdated,
            this,    &WhiteboardSession::objectsUpdated);

    connect(&m_transport, &NetworkTransport::deltaReceived,
            this,         &WhiteboardSession::onNetworkDelta);

    connect(&m_transport, &NetworkTransport::connected,
            this,         &WhiteboardSession::networkConnected);
    connect(&m_transport, &NetworkTransport::disconnected,
            this,         &WhiteboardSession::networkDisconnected);
    connect(&m_transport, &NetworkTransport::errorOccurred,
            this,         &WhiteboardSession::networkError);
    connect(&m_transport, &NetworkTransport::snapshotRequested,
            this,         &WhiteboardSession::onSnapshotRequested);
    connect(&m_transport, &NetworkTransport::peerCountChanged,
            this,         &WhiteboardSession::peerCountChanged);
}

void WhiteboardSession::connectToServer(const QUrl&    serverUrl,
                                        const QString& roomId,
                                        const QString& clientId)
{
    m_crdt.clear();
    m_transport.connectToServer(serverUrl, roomId, clientId);
}

void WhiteboardSession::disconnectFromServer()
{
    m_transport.disconnectFromServer();
}

bool WhiteboardSession::isOnline() const
{
    return m_transport.isConnected();
}

void WhiteboardSession::onLocalCreate(const DrawableObjectData& obj)
{
    QJsonObject delta = m_crdt.generateDelta("create", obj);
    m_crdt.applyDelta(delta);
    broadcastDelta(delta);
}

void WhiteboardSession::onLocalModify(const DrawableObjectData& obj)
{
    QJsonObject delta = m_crdt.generateDelta("modify", obj);
    m_crdt.applyDelta(delta);
    broadcastDelta(delta);
}

void WhiteboardSession::onLocalDelete(const DrawableObjectData& obj)
{
    QJsonObject delta = m_crdt.generateDelta("delete", obj);
    m_crdt.applyDelta(delta);
    broadcastDelta(delta);
}

void WhiteboardSession::onLocalDeleteAll()
{
    QJsonObject delta = m_crdt.generateDelta("deleteAll");
    m_crdt.applyDelta(delta);
    broadcastDelta(delta);
}

void WhiteboardSession::onNetworkDelta(const QJsonObject& delta)
{
    m_crdt.applyDelta(delta);

    emit deltaApplied(delta);
    emit objectsUpdated(m_crdt.getObjects());
}

void WhiteboardSession::onSnapshotRequested(const QString& peerId)
{
    QJsonArray snapshot;
    for (const auto& object : m_crdt.getObjects()) {
        snapshot.append(m_crdt.generateDelta("create", object));
    }
    m_transport.sendSnapshot(peerId, snapshot);
}

void WhiteboardSession::broadcastDelta(const QJsonObject& delta)
{
    emit deltaApplied(delta);

    m_transport.sendDelta(delta);
}
