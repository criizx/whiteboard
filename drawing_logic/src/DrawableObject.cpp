#include <utility>
#include <QtMath>

#include "../include/DrawableObject.h"

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

DrawableBrokenLine::DrawableBrokenLine(int id_, const std::vector<QPointF>& points_, int thickness_, QColor color_)
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
	const qreal lineLength = QLineF(start, end).length();
	if (lineLength == 0) {
		return QLineF(start, pos).length() <= thickness + brush_thickness;
	}

	const qreal u = ((pos.x() - start.x()) * (end.x() - start.x()) +
					(pos.y() - start.y()) * (end.y() - start.y())) /
				   (lineLength * lineLength);

	const QPointF closest = (u < 0) ? start :
						   (u > 1) ? end :
						   QPointF(start.x() + u * (end.x() - start.x()),
								  start.y() + u * (end.y() - start.y()));
	return QLineF(pos, closest).length() <= thickness +  brush_thickness;
}

bool DrawableBrokenLine::contains_point(QPointF pos, int brush_thickness) const {
	for (size_t i = 1; i < points.size(); ++i) {
		const QPointF& p1 = points[i-1];
		const QPointF& p2 = points[i];

		const qreal lineLength = QLineF(p1, p2).length();
		if (lineLength == 0) {
			if (QLineF(p1, pos).length() <= thickness) return true;
			continue;
		}

		const qreal u = ((pos.x() - p1.x()) * (p2.x() - p1.x()) +
						(pos.y() - p1.y()) * (p2.y() - p1.y())) /
					   (lineLength * lineLength);

		const QPointF closest = (u < 0) ? p1 :
							  (u > 1) ? p2 :
							  QPointF(p1.x() + u * (p2.x() - p1.x()),
									 p1.y() + u * (p2.y() - p1.y()));

		if (QLineF(pos, closest).length() <= thickness) {
			return true;
		}
	}
	return false;
}

bool DrawableRectangle::contains_point(QPointF pos, int brush_thickness) const {
	QRectF rect(start, end);
	rect = rect.normalized();
	return rect.adjusted(-thickness, -thickness, thickness, thickness)
			  .contains(pos);
}

