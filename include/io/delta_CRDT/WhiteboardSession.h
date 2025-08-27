#pragma once
#include <QObject>

#include <io/Delta_CRDT/CRDT.h>


class WhiteboardSession : public QObject {
	Q_OBJECT

public:
	explicit WhiteboardSession(QObject* parent = nullptr);


public slots:

	void onLocalCreate(const DrawableObjectData& obj);
	void onLocalModify(const DrawableObjectData& obj);
	void onLocalDelete(const DrawableObjectData& obj);
	void onLocalDeleteAll();

	void onNetworkDelta(const QJsonObject& delta);

signals:
	void deltaApplied(const QJsonObject& delta);
	void objectsUpdated(const QVector<DrawableObjectData>& objects);


private:
	DeltaCRDT m_crdt;
	int m_localCounter = 0;

	void broadcastDelta(const QJsonObject& delta);
};