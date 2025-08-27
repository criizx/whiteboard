#include <QJsonObject>

#include <io/Delta_CRDT/CRDT.h>
#include <Shared/Shared.h>

/*Delta

{
  "action": "create" | "modify" | "delete",
	id : {
		obj full data
	},
  "timestamp": 1692100005000
}

*/

void DeltaCRDT::applyDelta(const QJsonObject& delta) {
	QMutexLocker locker(&m_mutex);

	if (delta.isEmpty()) {
		return;
	}

	QString action = delta.value("action").toString();

	if (action == "deleteAll") {
		m_objects.clear();
		m_idToIndex.clear();
		emit allObjectsDeleted();
		emit objectsUpdated(m_objects);
		return;
	}

	qint64 ts = delta.value("timestamp").toVariant().toLongLong();

	QString dataId;
	QJsonObject dataObject;
	auto it = delta.constBegin();
	++it;

	for (auto it = delta.begin(); it != delta.end(); ++it) {
		if (it.key() == "action" || it.key() == "timestamp") continue;
		dataId = it.key();
		dataObject = it.value().toObject();
		break;
	}


	if (action == "create") {
		if (m_idToIndex.contains(dataId)) {
			qWarning() << "Duplicate create action for id:" << dataId;
			return;
		}

		DrawableObjectData obj;
		obj.id = dataId;
		obj.type = static_cast<ObjType>(dataObject["type"].toInt());
		obj.properties = dataObject;
		obj.timestamp = ts;
		m_objects.append(obj);
		m_idToIndex[obj.id] = m_objects.size() - 1;

		emit objectCreated(dataId, dataObject, ts);

	}else if (action == "modify") {
		if (!m_idToIndex.contains(dataId)) {
			qWarning() << "Smth went wrong no obj with this id:" << dataId;
			return;
		}
		int index = m_idToIndex[dataId];
		DrawableObjectData& existing = m_objects[index];
		if (ts < existing.timestamp) {
			return;
		}
		existing.id = dataId;
		existing.type = static_cast<ObjType>(dataObject["type"].toInt());
		existing.properties = dataObject;
		existing.timestamp = ts;
		
		m_objects[index] = existing;

		emit objectModified(dataId, dataObject, ts);

	}
	else if (action == "delete") {
		m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
			[&](const DrawableObjectData& o) { return o.id == dataId; }),
			m_objects.end());
		for (int i = 0; i < m_objects.size(); ++i) {
			m_idToIndex[m_objects[i].id] = i;
		}
		emit objectDeleted(dataId);

	}
	else {
		qWarning() << "Unknown action type in delta:" << action;
		return;
	}
	emit objectsUpdated(m_objects);
}

QJsonObject DeltaCRDT::generateDelta(const QString operation, const DrawableObjectData& obj){
	QJsonObject delta;
	delta["action"] = operation;

	if (operation != "deleteAll") {
		delta[obj.id] = obj.properties;
		delta["timestamp"] = obj.timestamp;
	}

	return delta;
}

void DeltaCRDT::updateDrawableObjs(){}

