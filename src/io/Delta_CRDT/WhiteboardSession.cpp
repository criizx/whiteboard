#include <io/Delta_CRDT/WhiteboardSession.h>
#include <Shared/Shared.h>

WhiteboardSession::WhiteboardSession(QObject* parent)
    : QObject(parent) {
    connect(&m_crdt, &DeltaCRDT::objectsUpdated,
        this, &WhiteboardSession::objectsUpdated);
}

void WhiteboardSession::onLocalCreate(const DrawableObjectData& obj){
    QJsonObject delta = m_crdt.generateDelta("create", obj);
    m_crdt.applyDelta(delta);

    broadcastDelta(delta);
}

void WhiteboardSession::onLocalModify(const DrawableObjectData& obj){
    QJsonObject delta = m_crdt.generateDelta("modify", obj);
    m_crdt.applyDelta(delta);

    broadcastDelta(delta);
}

void WhiteboardSession::onLocalDelete(const DrawableObjectData& obj){
    QJsonObject delta = m_crdt.generateDelta("delete", obj);
    m_crdt.applyDelta(delta);

    broadcastDelta(delta);
}

void WhiteboardSession::broadcastDelta(const QJsonObject& delta)
{
    emit deltaApplied(delta);
}

void WhiteboardSession::onNetworkDelta(const QJsonObject& delta) {
    m_crdt.applyDelta(delta);

    emit deltaApplied(delta);
    emit objectsUpdated(m_crdt.getObjects());
}

void WhiteboardSession::onLocalDeleteAll() {
    QJsonObject delta = m_crdt.generateDelta("deleteAll");
    m_crdt.applyDelta(delta);
    broadcastDelta(delta);
}
