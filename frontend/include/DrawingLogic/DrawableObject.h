#pragma once

#include <memory>
#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QJsonObject>
#include <utility>
#include <vector>
#include <QByteArray>

#include <Shared/Shared.h>


class DrawableObject {
protected:
    QString id;
    int thickness;
    QColor color;
    QBrush fill;

public:
    explicit DrawableObject(const QString id_, const int thickness_ = 3, QColor color_ = Qt::black, const QBrush& fill_ = Qt::NoBrush)
        : id(id_), thickness(thickness_), color(std::move(color_)), fill(fill_) {}

    virtual ~DrawableObject() = default;

    [[nodiscard]] QString get_id() const { return id; }
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

    virtual QJsonObject toJson() const;
    static std::shared_ptr<DrawableObject> fromJson(const QString id, const ObjType type, const QJsonObject& json);

    virtual QByteArray toBin() const = 0;
    static std::shared_ptr<DrawableObject> fromBin(QDataStream& stream, ObjType type);

    virtual DrawableObjectData toDrawableObjectData() const;
    static std::shared_ptr<DrawableObject> fromDrawableObjectData(const DrawableObjectData& data);

};

class DrawableLine : public DrawableObject {
    QPointF start, end;

public:
    DrawableLine(QString id_, QPointF s, QPointF e, int thickness_ = 3, QColor color_ = Qt::black)
        : DrawableObject(id_, thickness_, std::move(color_)), start(s), end(e) {}

    [[nodiscard]] QPointF get_start() const { return start; }
    [[nodiscard]] QPointF get_end() const override { return end; }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;

    QJsonObject toJson() const override;
    static std::shared_ptr<DrawableObject> fromJson(const QString id, const QJsonObject& json);

    QByteArray toBin() const ;
    static std::shared_ptr<DrawableObject> fromBin(QDataStream& stream);

    DrawableObjectData toDrawableObjectData() const override;
    static std::shared_ptr<DrawableObject> fromDrawableObjectData(const DrawableObjectData& data);
};

class DrawableBrokenLine : public DrawableObject {
    QVector<QPointF> points;
    QPainterPath path;

public:
    DrawableBrokenLine(QString id_, const QVector<QPointF>& points_, int thickness_ = 3, QColor color_ = Qt::black);

    [[nodiscard]] QPointF get_end() const override { return points.back(); }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;

    QJsonObject toJson() const override;
    static std::shared_ptr<DrawableObject> fromJson(const QString id, const QJsonObject& json);

    QByteArray toBin() const override;
    static std::shared_ptr<DrawableObject> fromBin(QDataStream& stream);

    DrawableObjectData toDrawableObjectData() const override;
    static std::shared_ptr<DrawableObject> fromDrawableObjectData(const DrawableObjectData& data);
private:
    void rebuild_path();
};

class DrawableRectangle : public DrawableObject {
    QPointF start, end;

public:
    DrawableRectangle(QString id_, QPointF s, QPointF e, int thickness_ = 3, QColor color_ = Qt::black, const QBrush& fill_ = Qt::NoBrush)
        : DrawableObject(id_, thickness_, std::move(color_), fill_), start(s), end(e) {}

    [[nodiscard]] QPointF get_end() const override { return end; }

    void draw(QPainter& painter) const override;
    void move_by(QPointF delta) override;
    [[nodiscard]] std::shared_ptr<DrawableObject> clone() const override;
	[[nodiscard]] bool contains_point(QPointF pos, int thickness) const override;

    QJsonObject toJson() const override;
    static std::shared_ptr<DrawableObject> fromJson(const QString id, const QJsonObject& json);

    QByteArray toBin() const override;
    static std::shared_ptr<DrawableObject> fromBin(QDataStream& stream);

    DrawableObjectData toDrawableObjectData() const override;
    static std::shared_ptr<DrawableObject> fromDrawableObjectData(const DrawableObjectData& data);
};

class DrawableAssistCircle : public DrawableObject {
	QPointF center;

    QJsonObject toJson() const override {
        return DrawableObject::toJson();
    }

    QByteArray toBin() const override {
        return QByteArray("lol");
    }

    DrawableObjectData toDrawableObjectData() const override {
        return DrawableObjectData();
    }

public:
	DrawableAssistCircle(QString id_, QPointF center_, int thickness_, QColor color_ = Qt::black)
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

