#pragma once

#include <QObject>
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QMutex>
#include <memory>

#include <Shared/Shared.h>

class DeltaCRDT : public QObject {
    Q_OBJECT

    mutable QMutex m_mutex;
    QVector<DrawableObjectData> m_objects;          
    QHash<QString, int> m_idToIndex;

public:

	void applyDelta(const QJsonObject& delta);

	QJsonObject generateDelta(const QString operation, const DrawableObjectData& obj = DrawableObjectData());

	void updateDrawableObjs();

    [[nodiscard]] const QVector<DrawableObjectData>& getObjects() const { return m_objects; }

signals:
    void objectCreated(const QString& id, const QJsonObject& properties, qint64 timestamp);
    void objectModified(const QString& id, const QJsonObject& properties, qint64 timestamp);
    void allObjectsDeleted();
    void objectDeleted(const QString& id);
    void objectsUpdated(const QVector<DrawableObjectData>& allObjects);
};
