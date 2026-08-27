#include <cmath>
#include <QDebug>
#include <algorithm>
#include <QMouseEvent>
#include <QPainter>
#include <unordered_map>

#include <DrawingLogic/CanvasWidget.h>
#include <DrawingLogic/Drawer.h>

CanvasWidget::CanvasWidget(QWidget* parent, const QString& UserId)
	: QWidget(parent), m_userId(UserId) {
	setObjectName("canvas");
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);
	setCursor(Qt::CrossCursor);
	setAutoFillBackground(true);
	auto canvas_palette = palette();
	canvas_palette.setColor(QPalette::Window, QColor("#fbfbfc"));
	setPalette(canvas_palette);
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
	painter.fillRect(rect(), QColor("#fbfbfc"));

	QTransform transform;
	transform.translate(m_offset.x(), m_offset.y());
	transform.scale(m_scale, m_scale);
	painter.setTransform(transform);

	if (m_grid_visible) {
		const QPointF top_left = to_world(QPointF(0, 0));
		const QPointF bottom_right = to_world(QPointF(width(), height()));
		constexpr qreal grid_step = 25.0;
		const qreal left = std::floor(top_left.x() / grid_step) * grid_step;
		const qreal top = std::floor(top_left.y() / grid_step) * grid_step;
		for (qreal x = left; x <= bottom_right.x(); x += grid_step) {
			const bool major = std::abs(std::fmod(x, grid_step * 4.0)) < 0.01;
			painter.setPen(QPen(major ? QColor("#d9dde5") : QColor("#eceef2"), 0));
			painter.drawLine(QPointF(x, top), QPointF(x, bottom_right.y()));
		}
		for (qreal y = top; y <= bottom_right.y(); y += grid_step) {
			const bool major = std::abs(std::fmod(y, grid_step * 4.0)) < 0.01;
			painter.setPen(QPen(major ? QColor("#d9dde5") : QColor("#eceef2"), 0));
			painter.drawLine(QPointF(left, y), QPointF(bottom_right.x(), y));
		}
	}

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
		setCursor(Qt::ClosedHandCursor);
		return;
	}

	if (event->button() == Qt::LeftButton && m_drawer) {
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
		setCursor(m_idle_cursor);
		return;
	}

	if (event->button() == Qt::LeftButton && m_drawer) {
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
	m_scale = std::clamp(m_scale * scale_factor, 0.2, 5.0);

	const QPointF after_scale = to_world(cursor_pos);
	m_offset += (after_scale - before_scale) * m_scale;

	update();
	emit viewChanged(m_scale);
	event->accept();
}

void CanvasWidget::reset_view() {
	m_scale = 1.0;
	m_offset = {0, 0};
	update();
	emit viewChanged(m_scale);
}

void CanvasWidget::zoom_in() {
	const QPointF center(width() / 2.0, height() / 2.0);
	const QPointF before_scale = to_world(center);
	m_scale = std::min(m_scale * 1.2, 5.0);
	m_offset += (to_world(center) - before_scale) * m_scale;
	update();
	emit viewChanged(m_scale);
}

void CanvasWidget::zoom_out() {
	const QPointF center(width() / 2.0, height() / 2.0);
	const QPointF before_scale = to_world(center);
	m_scale = std::max(m_scale / 1.2, 0.2);
	m_offset += (to_world(center) - before_scale) * m_scale;
	update();
	emit viewChanged(m_scale);
}

void CanvasWidget::set_grid_visible(bool visible) {
	m_grid_visible = visible;
	update();
}

void CanvasWidget::set_idle_cursor(Qt::CursorShape shape) {
	m_idle_cursor = shape;
	if (!m_panning) setCursor(shape);
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

bool CanvasWidget::remove_object(std::shared_ptr<DrawableObject> object) {
	auto it = std::find(m_objects.begin(), m_objects.end(), object);

	if (it != m_objects.end()) {
		auto removed_object = *it;
		m_objects.erase(it);
		emit objectDeleted(removed_object);
		return true;
	}
	return false;
}

void CanvasWidget::notify_object_modified(const std::shared_ptr<DrawableObject>& object) {
	if (object) {
		emit objectModified(object);
	}
}
