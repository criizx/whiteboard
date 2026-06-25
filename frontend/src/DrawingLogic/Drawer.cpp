#include <ranges>
#include <utility>

#include <DrawingLogic/CanvasWidget.h>
#include <DrawingLogic/DrawableObject.h>
#include <DrawingLogic/Drawer.h>

void BrokenLineDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_points.clear();
    m_points.push_back(pos);
    m_drawing = true;
}

void BrokenLineDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
	if (!m_drawing) {return;}
	m_points.push_back(pos);
	preview_path = std::make_shared<DrawableBrokenLine>("__preview__", m_points, thickness, color);

	canvas->setPreview(canvas->getUserId(), preview_path);
}


void BrokenLineDrawer::on_mouse_release(CanvasWidget* canvas, const QPointF pos) {
	if (!m_drawing) {return;}
	m_points.push_back(pos);

	QString id = canvas->generate_id();

	canvas->addObject(std::make_shared<DrawableBrokenLine>(id, m_points, thickness, color));
	m_drawing = false;
	canvas->clearPreview(canvas->getUserId());
}


void LineDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_drawing = true;
    start_point = pos;
    end_point = pos;

    preview_line = std::make_shared<DrawableLine>("__preview__", start_point, end_point, thickness, color);
	canvas->setPreview(canvas->getUserId(), preview_line);
}

void LineDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing || !preview_line) {
	    return;
    }

    end_point = pos;
    preview_line = std::make_shared<DrawableLine>("__preview__", start_point, end_point, thickness, color);

	canvas->setPreview(canvas->getUserId(), preview_line);
}

void LineDrawer::on_mouse_release(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing) return;
    end_point = pos;

   
    QString id = canvas->generate_id();
	canvas->addObject(std::make_shared<DrawableLine>(id, start_point, end_point, thickness, color));

    m_drawing = false;
    canvas->clearPreview(canvas->getUserId());
}

void RectangleDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_drawing = true;
    start_point = pos;
    end_point = pos;

    preview_rectangle = std::make_shared<DrawableRectangle>("__preview__", start_point, end_point, thickness, color);
	canvas->setPreview(canvas->getUserId(), preview_rectangle);
}

void RectangleDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing || !preview_rectangle) return;

    end_point = pos;
    preview_rectangle = std::make_shared<DrawableRectangle>("__preview__", start_point, end_point, thickness, color);

	canvas->setPreview(canvas->getUserId(), preview_rectangle);
}

void RectangleDrawer::on_mouse_release(CanvasWidget* canvas, QPointF pos) {
    if (!m_drawing) return;

    end_point = pos;

    QString id = canvas->generate_id();
	canvas->addObject(std::make_shared<DrawableRectangle>(id, start_point, end_point, thickness, color));

    m_drawing = false;
	canvas->clearPreview(canvas->getUserId());
}

void EraserTool::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
	is_erasing = true;
	center = pos;

	preview_circle = std::make_shared<DrawableAssistCircle>("__preview__", center, thickness, Qt::black);

	canvas->setToolPreview(preview_circle);

	erase_at(canvas, pos, thickness);
	canvas->update();
}

void EraserTool::on_mouse_move(CanvasWidget* canvas, QPointF pos) {
	if (is_erasing) {
		center = pos;
		erase_at(canvas, pos, thickness);

		preview_circle = std::make_shared<DrawableAssistCircle>("__preview__", center, thickness, Qt::black);

		canvas->setToolPreview(preview_circle);

		canvas->update();
	}
}

void EraserTool::on_mouse_release(CanvasWidget* canvas, QPointF) {
	is_erasing = false;

	auto& objs = canvas->objects();
	canvas->clearToolPreview();

	canvas->update();
}


void EraserTool::erase_at(CanvasWidget* canvas, QPointF pos, int brush_thickness) {
	auto& objs = canvas->objects();

	for (int i = objs.size() - 1; i >= 0; --i) {
		if (objs[i]->contains_point(pos, brush_thickness)) {
			canvas->remove_object(objs[i]);
		}
	}
}

void MoveTool::on_mouse_press(CanvasWidget* canvas, QPointF pos) {
	for (const auto& obj : std::ranges::reverse_view(canvas->objects())) {
		if (obj->contains_point(pos, thickness)) {
			selected = obj;
			last_pos = pos;
			return;
		}
	}
	selected.reset();
}

void MoveTool::on_mouse_move(CanvasWidget*, QPointF pos) {
	if (selected) {
		QPointF delta = pos - last_pos;
		selected->move_by(delta);
		last_pos = pos;
	}
}

void MoveTool::on_mouse_release(CanvasWidget*, QPointF) {
	selected.reset();
}
