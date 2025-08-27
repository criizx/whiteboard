#pragma once

#include <QWidget>

#include <DrawingLogic/DrawableObject.h>
#include <DrawingLogic/Drawer.h>


class CanvasWidget final : public QWidget {
	Q_OBJECT

public:
	explicit CanvasWidget(QWidget* parent = nullptr, const QString& userId = QString());
	void set_drawer(std::unique_ptr<Drawer> drawer);

	[[nodiscard]] const std::vector<std::shared_ptr<DrawableObject>>& objects() const;

	void setPreview(QString UserId, std::shared_ptr<DrawableObject> preview);
	void clearPreview(QString UserId);
	void clearAllPreviews();

	void setToolPreview(std::shared_ptr<DrawableObject> preview);
	void clearToolPreview();
	
	QString getUserId() {
		return m_userId;
	}
	void addObject(std::shared_ptr<DrawableObject> obj);
	bool remove_object(const std::shared_ptr<DrawableObject>& object);

protected:
	void paintEvent(QPaintEvent*) override;

	void mousePressEvent(QMouseEvent*) override;
	void mouseMoveEvent(QMouseEvent*) override;
	void mouseReleaseEvent(QMouseEvent*) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	std::vector<std::shared_ptr<DrawableObject>> m_objects;
	std::unique_ptr<Drawer> m_drawer;
	int m_next_id = 0;
	QString m_userId;
	
	std::unordered_map<QString, std::shared_ptr<DrawableObject>> m_previews;
	std::shared_ptr<DrawableObject> m_tool_preview;

	QColor m_pen_color = Qt::black;
	int m_pen_thickness = 3;
	QBrush m_fill = Qt::NoBrush;

	QPointF m_offset = {0, 0};
	qreal m_scale = 1.0;
	QPoint m_last_pan_pos;
	bool m_panning = false;

	void create_drawer_by_name(const QString& name);

signals:
	void objectCreated(std::shared_ptr<DrawableObject> obj);
	void objectDeleted(std::shared_ptr<DrawableObject> obj);
	void objectModified(std::shared_ptr<DrawableObject> obj);
	void allObjectsDeleted();

public:
	QString generate_id();
	void set_pen_color(const QColor& color);
	void set_fill(QBrush b);
	void set_pen_thickness(int value);
	void set_tool(const QString& name);
	void clear_all();

	[[nodiscard]] QPointF to_world(const QPointF& screen_pos) const;
	[[nodiscard]] QPointF to_screen(const QPointF& world_pos) const;
};
