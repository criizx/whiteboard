#include <QIODevice>
#include <QJsonArray>
#include <utility>

#include <DrawingLogic/DrawableObject.h>

QString pointFToSerializedString(const QPointF& point) {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << point;
    return QString::fromLatin1(byteArray.toBase64());
}

QPointF serializedStringToPointF(const QString& str) {
    QByteArray byteArray = QByteArray::fromBase64(str.toLatin1());
    QDataStream stream(&byteArray, QIODevice::ReadOnly);
    QPointF point;
    stream >> point;
    return point;
}

QString colorToSerializedString(const QColor& color) {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);
    stream << color;
    return QString::fromLatin1(byteArray.toBase64());
}

QColor serializedStringToColor(const QString& str) {
    QByteArray byteArray = QByteArray::fromBase64(str.toLatin1());
    QDataStream stream(&byteArray, QIODevice::ReadOnly);
    QColor color;
    stream >> color;
    return color;
}

QJsonArray pointsToJson(const QVector<QPointF>& points) {
    QJsonArray array;
    for (const QPointF& point : points) {
        QJsonObject pt;
        pt["x"] = point.x();
        pt["y"] = point.y();
        array.append(pt);
    }
    return array;
}

QVector<QPointF> jsonToPoints(const QJsonArray& jsonArray) {
    QVector<QPointF> points;
    points.reserve(jsonArray.size());
    for (const QJsonValue& value : jsonArray) {
        QJsonObject pt = value.toObject();
        if (pt.contains("x") && pt.contains("y")) {
            double x = pt["x"].toDouble();
            double y = pt["y"].toDouble();
            if (!std::isnan(x) && !std::isnan(y)) {
                points.emplace_back(x, y);
            }
            else {
                qWarning() << "Invalid point coords in json";
            }
        }
        else {
            qWarning() << "Invalid point obj in json";
        }
    }
    return points;
}

void DrawableLine::draw(QPainter& painter) const {
	const QPen pen(color, thickness);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawLine(start, end);
}

void DrawableLine::move_by(const QPointF delta) {
	start += delta;
	end += delta;
}

std::shared_ptr<DrawableObject> DrawableLine::clone() const {
	return std::make_shared<DrawableLine>(id, start, end, thickness, color);
}

DrawableBrokenLine::DrawableBrokenLine(QString id_, const QVector<QPointF>& points_, int thickness_, QColor color_)
	: DrawableObject(id_, thickness_, std::move(color_)), points(points_) {
	rebuild_path();
}

void DrawableBrokenLine::draw(QPainter& painter) const {
	QPen pen(color, thickness);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawPath(path);
}

void DrawableBrokenLine::move_by(QPointF delta) {
	for (auto& pt : points) {
		pt += delta;
	}
	rebuild_path();
}

std::shared_ptr<DrawableObject> DrawableBrokenLine::clone() const {
	return std::make_shared<DrawableBrokenLine>(id, points, thickness, color);
}

void DrawableBrokenLine::rebuild_path() {
	path = QPainterPath();
	if (!points.empty()) {
		path.moveTo(points[0]);
		for (size_t i = 1; i < points.size(); ++i) {
			path.lineTo(points[i]);
		}
	}
}

void DrawableRectangle::draw(QPainter& painter) const {
	QPen pen(color, thickness);
	painter.setPen(pen);
	painter.setBrush(fill);
	painter.drawRect(QRectF(start, end));
}

void DrawableRectangle::move_by(const QPointF delta) {
	start += delta;
	end += delta;
}

std::shared_ptr<DrawableObject> DrawableRectangle::clone() const {
	return std::make_shared<DrawableRectangle>(id, start, end, thickness, color, fill);
}

bool DrawableLine::contains_point(QPointF pos, int brush_thickness) const {
    const qreal threshold = (thickness / 2.0) + static_cast<qreal>(brush_thickness);

    const qreal lineLength = QLineF(start, end).length();
    if (lineLength == 0) {
        return QLineF(start, pos).length() <= threshold;
    }

    const qreal u = ((pos.x() - start.x()) * (end.x() - start.x()) +
        (pos.y() - start.y()) * (end.y() - start.y())) /
        (lineLength * lineLength);

    const QPointF closest = (u < 0) ? start :
        (u > 1) ? end :
        QPointF(start.x() + u * (end.x() - start.x()),
            start.y() + u * (end.y() - start.y()));
    return QLineF(pos, closest).length() <= threshold;
}

bool DrawableBrokenLine::contains_point(QPointF pos, int brush_thickness) const {
    if (points.empty()) return false;

    const qreal threshold = (thickness / 2.0) + static_cast<qreal>(brush_thickness);

    if (points.size() == 1) {
        return QLineF(points[0], pos).length() <= threshold;
    }

    for (size_t i = 1; i < points.size(); ++i) {
        const QPointF& p1 = points[i - 1];
        const QPointF& p2 = points[i];

        const qreal lineLength = QLineF(p1, p2).length();
        if (lineLength == 0) {
            if (QLineF(p1, pos).length() <= threshold) return true;
            continue;
        }

        const qreal u = ((pos.x() - p1.x()) * (p2.x() - p1.x()) +
            (pos.y() - p1.y()) * (p2.y() - p1.y())) /
            (lineLength * lineLength);

        const QPointF closest = (u < 0) ? p1 :
            (u > 1) ? p2 :
            QPointF(p1.x() + u * (p2.x() - p1.x()),
                p1.y() + u * (p2.y() - p1.y()));

        if (QLineF(pos, closest).length() <= threshold) {
            return true;
        }
    }
    return false;
}

bool DrawableRectangle::contains_point(QPointF pos, int brush_thickness) const {
    QRectF rect(start, end);
    rect = rect.normalized();
    const qreal threshold = (thickness / 2.0) + static_cast<qreal>(brush_thickness);
    return rect.adjusted(-threshold, -threshold, threshold, threshold).contains(pos);
}

// base obj
QJsonObject DrawableObject::toJson() const {

    QJsonObject inner;
    inner["thickness"] = thickness;
    inner["color"] = colorToSerializedString(color);

    QJsonObject top;
    top[id] = inner;
    return top;
}

DrawableObjectData DrawableObject::toDrawableObjectData() const {
    DrawableObjectData result;
    result.id = id;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();
    return result;
 }

std::shared_ptr<DrawableObject> DrawableObject::fromDrawableObjectData(const DrawableObjectData& data) {
    auto type = data.type;
    if (type == ObjType::Line) {
        return DrawableLine::fromDrawableObjectData(data);
    }
    if (type == ObjType::BrokenLine) {
        return DrawableBrokenLine::fromDrawableObjectData(data);
    }
    if (type == ObjType::Rectangle) {
        return DrawableRectangle::fromDrawableObjectData(data);
    }
    qWarning() << "Unknown obj type:" << static_cast<int>(type);
    return nullptr;
}

std::shared_ptr<DrawableObject> DrawableObject::fromJson(const QString id, const ObjType type, const QJsonObject& json) {
    if (type == ObjType::Line) {
        return DrawableLine::fromJson(id, json);
    }
    if (type == ObjType::BrokenLine) {
        return DrawableBrokenLine::fromJson(id, json);
    }
    if (type == ObjType::Rectangle) {
        return DrawableRectangle::fromJson(id, json);
    }

    qWarning() << "Unknown obj type:" << static_cast<int>(type);
    return nullptr;
}

std::shared_ptr<DrawableObject> DrawableObject::fromBin(QDataStream& stream, ObjType type) {
    if (type == ObjType::Line) {
        return DrawableLine::fromBin(stream);
    }
    if (type == ObjType::BrokenLine) {
        return DrawableBrokenLine::fromBin(stream);
    }
    if (type == ObjType::Rectangle) {
        return DrawableRectangle::fromBin(stream);
    }
    qWarning() << "Unknown obj type:" << static_cast<int>(type);
    return nullptr;
}

//line
QJsonObject DrawableLine::toJson() const {
    QJsonObject json = DrawableObject::toJson();

    QJsonObject objData = json[id].toObject();
    objData["type"] = static_cast<int>(ObjType::Line);
    objData["start"] = pointFToSerializedString(start);
    objData["end"] = pointFToSerializedString(end);

    json[id] = objData;
    return json;
}

std::shared_ptr<DrawableObject> DrawableLine::fromJson(const QString id, const QJsonObject& json) {

    if (!json.contains("start") || !json.contains("end") ||
        !json.contains("thickness") || !json.contains("color")) {
        qWarning() << "Invalid json for line: missing fields";
        return nullptr;
    }

    QPointF start = serializedStringToPointF(json["start"].toString());
    QPointF end = serializedStringToPointF(json["end"].toString());
    int thickness = json["thickness"].toInt();
    QColor color = serializedStringToColor(json["color"].toString());

    if (id.isEmpty() || thickness <= 0 || !color.isValid()) {
        qWarning() << "Invalid data in line json";
        return nullptr;
    }

    return std::make_shared<DrawableLine>(id, start, end, thickness, color);
}

QByteArray DrawableLine::toBin() const {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);

    stream << ObjType::Line;
    stream << id;
    stream << thickness;
    stream << color;
    stream << start;
    stream << end;

    return byteArray;
}

std::shared_ptr<DrawableObject> DrawableLine::fromBin(QDataStream& stream) {
    QString id;
    int thickness;
    QColor color;
    QPointF start;
    QPointF end;

    stream >> id;
    stream >> thickness;
    stream >> color;
    stream >> start;
    stream >> end;

    return std::make_shared<DrawableLine>(id, start, end, thickness, color);
}

DrawableObjectData DrawableLine::toDrawableObjectData() const {
    auto json = toJson();
    DrawableObjectData result = DrawableObject::toDrawableObjectData();
    result.type = ObjType::Line;
    result.properties = json[id].toObject();
    return result;
}

std::shared_ptr<DrawableObject> DrawableLine::fromDrawableObjectData(const DrawableObjectData& data) {
    return DrawableObject::fromJson(data.id, data.type, data.properties);
}

//broken line
QJsonObject DrawableBrokenLine::toJson() const {
    QJsonObject json = DrawableObject::toJson();

    QJsonObject objData = json[id].toObject();
    objData["type"] = static_cast<int>(ObjType::BrokenLine);
    objData["points"] = pointsToJson(points);

    json[id] = objData;
    return json;
}

std::shared_ptr<DrawableObject> DrawableBrokenLine::fromJson(const QString id, const QJsonObject& json) {

    if (!json.contains("points") || !json.contains("thickness") || !json.contains("color")) {
        qWarning() << "Invalid json for broken line: missing fields";
        return nullptr;
    }

    QVector<QPointF> points = jsonToPoints(json["points"].toArray());
    int thickness = json["thickness"].toInt();
    QColor color = serializedStringToColor(json["color"].toString());

    if (id.isEmpty() || points.empty() || thickness <= 0 || !color.isValid()) {
        qWarning() << "Invalid data in broken line json";
        return nullptr;
    }
    return std::make_shared<DrawableBrokenLine>(id, points, thickness, color);
}

QByteArray DrawableBrokenLine::toBin() const {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);

    stream << ObjType::BrokenLine;
    stream << id;
    stream << thickness;
    stream << color;
    stream << points;
    return byteArray;
}

std::shared_ptr<DrawableObject> DrawableBrokenLine::fromBin(QDataStream& stream) {
    QString id;
    int thickness;
    QColor color;
    QVector<QPointF> points;

    stream >> id;
    stream >> thickness;
    stream >> color;
    stream >> points;

    return std::make_shared<DrawableBrokenLine>(id, points, thickness, color);
}

DrawableObjectData DrawableBrokenLine::toDrawableObjectData() const {
    auto json = toJson();
    DrawableObjectData result = DrawableObject::toDrawableObjectData();
    result.type = ObjType::BrokenLine;
    result.properties = json[id].toObject();
    return result;
}

std::shared_ptr<DrawableObject> DrawableBrokenLine::fromDrawableObjectData(const DrawableObjectData& data) {
    return DrawableObject::fromJson(data.id, data.type, data.properties);
}

//rectangle
QJsonObject DrawableRectangle::toJson() const {
    QJsonObject json = DrawableObject::toJson();

    QJsonObject objData = json[id].toObject();
    objData["type"] = static_cast<int>(ObjType::Rectangle);
    objData["start"] = pointFToSerializedString(start);
    objData["end"] = pointFToSerializedString(end);

    json[id] = objData;
    return json;
}

std::shared_ptr<DrawableObject> DrawableRectangle::fromJson(const QString id, const QJsonObject& json) {
    if (!json.contains("start") || !json.contains("end") ||
        !json.contains("thickness") || !json.contains("color")) {
        qWarning() << "Invalid json for rect: missing fields";
        return nullptr;
    }

    QPointF start = serializedStringToPointF(json["start"].toString());
    QPointF end = serializedStringToPointF(json["end"].toString());
    int thickness = json["thickness"].toInt();
    QColor color = serializedStringToColor(json["color"].toString());

    if (id.isEmpty() || thickness <= 0 || !color.isValid()) {
        qWarning() << "Invalid data in rect json";
        return nullptr;
    }
    return std::make_shared<DrawableRectangle>(id, start, end, thickness, color);
}

QByteArray DrawableRectangle::toBin() const {
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);

    stream << ObjType::Rectangle;
    stream << id;
    stream << thickness;
    stream << color;
    stream << start;
    stream << end;

    return byteArray;
}

std::shared_ptr<DrawableObject> DrawableRectangle::fromBin(QDataStream& stream) {
    QString id;
    int thickness;
    QColor color;
    QPointF start;
    QPointF end;

    stream >> id;
    stream >> thickness;
    stream >> color;
    stream >> start;
    stream >> end;

    return std::make_shared<DrawableRectangle>(id, start, end, thickness, color);
}

DrawableObjectData DrawableRectangle::toDrawableObjectData() const {
    auto json = toJson();
    DrawableObjectData result = DrawableObject::toDrawableObjectData();
    result.type = ObjType::Rectangle;
    result.properties = json[id].toObject();
    return result;
}

std::shared_ptr<DrawableObject> DrawableRectangle::fromDrawableObjectData(const DrawableObjectData& data) {
    return DrawableObject::fromJson(data.id, data.type, data.properties);
}