#include <cmath>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <unordered_map>

#include <DrawingLogic/CanvasWidget.h>
#include <DrawingLogic/Drawer.h>

CanvasWidget::CanvasWidget(QWidget* parent, const QString& UserId)
	: QWidget(parent), m_userId(UserId) {
	setMouseTracking(true);
	create_drawer_by_name("line");
}

const std::vector<std::shared_ptr<DrawableObject>>& CanvasWidget::objects() const {
	return m_objects;
}

void CanvasWidget::set_pen_color(const QColor& color) {
	m_pen_color = color;
	if (m_drawer) m_drawer->set_color(color);
}

void CanvasWidget::set_fill(QBrush b) {
	m_fill = b;
	if (m_drawer) m_drawer->set_fill(b);
}

void CanvasWidget::set_pen_thickness(int t) {
	m_pen_thickness = t;
	if (m_drawer) m_drawer->set_thickness(t);
}

void CanvasWidget::set_drawer(std::unique_ptr<Drawer> drawer) {
	m_drawer = std::move(drawer);
	m_drawer->set_color(m_pen_color);
	m_drawer->set_fill(m_fill);
	m_drawer->set_thickness(m_pen_thickness);
}
void CanvasWidget::set_tool(const QString& name) {
	create_drawer_by_name(name);
}

void CanvasWidget::create_drawer_by_name(const QString& name) {
	if (name == "line") {
		set_drawer(std::make_unique<LineDrawer>());
	} else if (name == "brush") {
		set_drawer(std::make_unique<BrokenLineDrawer>());
	} else if (name == "rectangle") {
		set_drawer(std::make_unique<RectangleDrawer>());
	} else if (name == "eraser") {
		set_drawer(std::make_unique<EraserTool>());
	} else {
		qDebug() << "Unknown tool:" << name;
	}
}

void CanvasWidget::clear_all() {
	m_objects.clear();
	update();
	emit allObjectsDeleted();
}

void CanvasWidget::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QTransform transform;
	transform.translate(m_offset.x(), m_offset.y());
	transform.scale(m_scale, m_scale);
	painter.setTransform(transform);

	for (const auto& obj : m_objects) {
		obj->draw(painter);
	}

	for (const auto& [id, preview] : m_previews) {
		if (preview) {
			preview->draw(painter);
		}
	}

	if (m_tool_preview)
	{
		m_tool_preview->draw(painter);
	}
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::MiddleButton) {
		m_panning = true;
		m_last_pan_pos = event->pos();
		return;
	}

	if (m_drawer) {
		m_drawer->on_mouse_press(this, to_world(event->pos()));
		update();
	}
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
	if (m_panning) {
		QPointF delta = event->pos() - m_last_pan_pos;
		m_offset += delta;
		m_last_pan_pos = event->pos();
		update();
		return;
	}

	if (m_drawer) {
		m_drawer->on_mouse_move(this, to_world(event->pos()));
		update();
	}
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::MiddleButton) {
		m_panning = false;
		return;
	}

	if (m_drawer) {
		m_drawer->on_mouse_release(this, to_world(event->pos()));
		update();
	}
}

QString CanvasWidget::generate_id() {
	m_next_id++;
	return m_userId + "-" + QString::number(m_next_id);
}

QPointF CanvasWidget::to_world(const QPointF& screen_pos) const {
	return (screen_pos - m_offset) / m_scale;
}

QPointF CanvasWidget::to_screen(const QPointF& world_pos) const {
	return world_pos * m_scale + m_offset;
}

void CanvasWidget::wheelEvent(QWheelEvent* event) {
	const QPointF cursor_pos = event->position();
	const QPointF before_scale = to_world(cursor_pos);

	const qreal scale_factor = std::pow(1.0015, event->angleDelta().y());
	m_scale *= scale_factor;

	const QPointF after_scale = to_world(cursor_pos);
	m_offset += (after_scale - before_scale) * m_scale;

	update();
}

void CanvasWidget::setPreview(QString UserId, std::shared_ptr<DrawableObject> preview){
	m_previews[UserId] = preview;
}
void CanvasWidget::clearPreview(QString UserId) {
	m_previews.erase(UserId);
}

void CanvasWidget::clearAllPreviews(){
	m_previews.clear();
}

void CanvasWidget::setToolPreview(std::shared_ptr<DrawableObject> preview) {
	m_tool_preview = preview;
}

void CanvasWidget::clearToolPreview() {
	m_tool_preview = nullptr;
}

void CanvasWidget::addObject(std::shared_ptr<DrawableObject> obj) {
	m_objects.push_back(obj);

	emit objectCreated(obj);
}

bool CanvasWidget::remove_object(const std::shared_ptr<DrawableObject>& object) {
	auto it = std::find(m_objects.begin(), m_objects.end(), object);

	if (it != m_objects.end()) {
		m_objects.erase(it);
		emit objectDeleted(object);
		return true;
	}
	return false;
}