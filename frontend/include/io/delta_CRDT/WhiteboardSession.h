#pragma once
#include <QObject>
#include <QUrl>

#include <io/delta_CRDT/CRDT.h>
#include <io/Network/NetworkTransport.h>


class WhiteboardSession : public QObject {
	Q_OBJECT

public:
	explicit WhiteboardSession(QObject* parent = nullptr);

    void connectToServer(const QUrl& serverUrl,
                         const QString& roomId,
                         const QString& clientId);

    void disconnectFromServer();

    [[nodiscard]] bool isOnline() const;

public slots:

	void onLocalCreate(const DrawableObjectData& obj);
	void onLocalModify(const DrawableObjectData& obj);
	void onLocalDelete(const DrawableObjectData& obj);
	void onLocalDeleteAll();

	void onNetworkDelta(const QJsonObject& delta);
	void onSnapshotRequested(const QString& peerId);

signals:
	void deltaApplied(const QJsonObject& delta);
	void objectsUpdated(const QVector<DrawableObjectData>& objects);

    void networkConnected();
    void networkDisconnected();
    void networkError(const QString& message);
    void peerCountChanged(int count);

private:
	DeltaCRDT          m_crdt;
    NetworkTransport   m_transport;
	int                m_localCounter = 0;

	void broadcastDelta(const QJsonObject& delta);
};
