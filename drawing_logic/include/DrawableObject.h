#pragma once

#include <QPointF>
#include <QPainterPath>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <memory>
#include <utility>
#include <vector>

class DrawableObject {
protected:
    int id;
    int thickness;
    QColor color;
    QBrush fill;

public:
    explicit DrawableObject(const int id_, const int thickness_ = 3, QColor color_ = Qt::black, const QBrush& fill_ = Qt::NoBrush)
        : id(id_), thickness(thickness_), color(std::move(color_)), fill(fill_) {}

    virtual ~DrawableObject() = default;

    [[nodiscard]] int get_id() const { return id; }
    [[nodiscard]] int get_thickness() const { return thickness; }
    void set_thickness(const int t) { thickness = t; }

    [[nodiscard]] QColor get_color() const { return color; }
    void set_color(const QColor& c) { color = c; }

    [[nodiscard]] QBrush get_fill() const { return fill; }
    void set_fill(const QBrush& f) { fill = f; }

    virtual void draw(QPainter& painter) const = 0;
    virtual void move_by(QPointF delta) = 0;
    [[nodiscard]] virtual std::shared_ptr<DrawableObject> clone() const = 0;
    [[nodiscard]] virtual QPointF get_end() const = 0;
	[[nodiscard]] virtual bool contains_point(QPointF pos, int thickness) const = 0;
};

class DrawableLine : public DrawableObject {
    QPointF start, end;

public:
    DrawableLine(int id_, QPointF s, QPointF e, int thickness_ = 3, QColor color_ = Qt::black)
        : DrawableObject(id_, thickness_, std::move(color_)), start(s), end(e) {}

    [[nodiscard]] QPointF get_start() const { return start; }
    [[nodiscard]] QPointF get_end() const override { return end; }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;
};

class DrawableBrokenLine : public DrawableObject {
    std::vector<QPointF> points;
    QPainterPath path;

public:
    DrawableBrokenLine(int id_, const std::vector<QPointF>& points_, int thickness_ = 3, QColor color_ = Qt::black);

    [[nodiscard]] QPointF get_end() const override { return points.back(); }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;

private:
    void rebuild_path();
};

class DrawableRectangle : public DrawableObject {
    QPointF start, end;

public:
    DrawableRectangle(int id_, QPointF s, QPointF e, int thickness_ = 3, QColor color_ = Qt::black, const QBrush& fill_ = Qt::NoBrush)
        : DrawableObject(id_, thickness_, std::move(color_), fill_), start(s), end(e) {}

    [[nodiscard]] QPointF get_end() const override { return end; }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;
};

class DrawableAssistCircle : public DrawableObject {
	QPointF center;

public:
	DrawableAssistCircle(int id_, QPointF center_, int thickness_, QColor color_ = Qt::black)
		: DrawableObject(id_, thickness_, std::move(color_)), center(center_) {}

	void draw(QPainter& painter) const override {
		QPen pen(color, 1);
		painter.setPen(pen);
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(center, thickness, thickness);
	}

	void move_by(QPointF delta) override { center += delta; }

	[[nodiscard]] std::shared_ptr<DrawableObject> clone() const override {
		return std::make_shared<DrawableAssistCircle>(id, center, thickness, color);
	}

	[[nodiscard]] bool contains_point(const QPointF pos, const int brush_thickness) const override {
		return QLineF(center, pos).length() <= thickness + brush_thickness;
	}

	[[nodiscard]] QPointF get_end() const override { return center; }

	[[nodiscard]] double get_radius() const { return thickness; }
	[[nodiscard]] QPointF get_center() const { return center; }
};

