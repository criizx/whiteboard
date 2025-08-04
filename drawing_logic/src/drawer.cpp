#include "../include/drawer.h"
#include "../include/canvaswidget.h"
#include "../include/DrawableObject.h"
#include <ranges>
#include <utility>


void BrokenLineDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_points.clear();
    m_points.push_back(pos);
    m_drawing = true;
}

void BrokenLineDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
	if (!m_drawing) {return;}
	m_points.push_back(pos);
	preview_path = std::make_shared<DrawableBrokenLine>(-1, m_points, thickness, color);
	auto& objs = canvas->objects();
	if (!objs.empty() && objs.back()->get_id() == -1) {
		objs.back() = preview_path;
	}
	else {
		objs.push_back(preview_path);
	}
}


void BrokenLineDrawer::on_mouse_release(CanvasWidget* canvas, const QPointF pos) {
	if (!m_drawing) {return;}
	m_points.push_back(pos);
	auto& objs = canvas->objects();
	if (!objs.empty() && objs.back()->get_id() == -1) {
		objs.pop_back();
	}
	int id = canvas->generate_id();
	objs.push_back(std::make_shared<DrawableBrokenLine>(id, m_points, thickness, color));
	m_drawing = false;
	preview_path.reset();
}


void LineDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_drawing = true;
    start_point = pos;
    end_point = pos;

    preview_line = std::make_shared<DrawableLine>(-1, start_point, end_point, thickness, color);
    canvas->objects().push_back(preview_line);
}

void LineDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing || !preview_line) {
	    return;
    }

    end_point = pos;
    preview_line = std::make_shared<DrawableLine>(-1, start_point, end_point, thickness, color);

    auto& objs = canvas->objects();
    if (!objs.empty() && objs.back()->get_id() == -1) {
        objs.back() = preview_line;
    }
}

void LineDrawer::on_mouse_release(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing) return;
    end_point = pos;

    auto& objs = canvas->objects();
    if (!objs.empty() && objs.back()->get_id() == -1)
        objs.pop_back();

    int id = canvas->generate_id();
    objs.push_back(std::make_shared<DrawableLine>(id, start_point, end_point, thickness, color));

    m_drawing = false;
    preview_line.reset();
}

void RectangleDrawer::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
    m_drawing = true;
    start_point = pos;
    end_point = pos;

    preview_rectangle = std::make_shared<DrawableRectangle>(-1, start_point, end_point, thickness, color);
    canvas->objects().push_back(preview_rectangle);
}

void RectangleDrawer::on_mouse_move(CanvasWidget* canvas, const QPointF pos) {
    if (!m_drawing || !preview_rectangle) return;

    end_point = pos;
    preview_rectangle = std::make_shared<DrawableRectangle>(-1, start_point, end_point, thickness, color);

    if (auto& objs = canvas->objects(); !objs.empty() && objs.back()->get_id() == -1)
        objs.back() = preview_rectangle;
}

void RectangleDrawer::on_mouse_release(CanvasWidget* canvas, QPointF pos) {
    if (!m_drawing) return;

    end_point = pos;
    auto& objs = canvas->objects();
    if (!objs.empty() && objs.back()->get_id() == -1)
        objs.pop_back();

    int id = canvas->generate_id();
    objs.push_back(std::make_shared<DrawableRectangle>(id, start_point, end_point, thickness, color));

    m_drawing = false;
    preview_rectangle.reset();
}

void EraserTool::on_mouse_press(CanvasWidget* canvas, const QPointF pos) {
	is_erasing = true;
	center = pos;

	preview_circle = std::make_shared<DrawableAssistCircle>(-1, center, thickness, Qt::black);

	auto& objs = canvas->objects();
	objs.push_back(preview_circle);

	erase_at(canvas, pos, thickness);
	canvas->update();
}

void EraserTool::on_mouse_move(CanvasWidget* canvas, QPointF pos) {
	if (is_erasing) {
		center = pos;
		erase_at(canvas, pos, thickness);

		preview_circle = std::make_shared<DrawableAssistCircle>(-1, center, thickness, Qt::black);

		auto& objs = canvas->objects();
		if (!objs.empty() && objs.back()->get_id() == -1)
			objs.back() = preview_circle;
		else
			objs.push_back(preview_circle);

		canvas->update();
	}
}

void EraserTool::on_mouse_release(CanvasWidget* canvas, QPointF) {
	is_erasing = false;

	auto& objs = canvas->objects();
	if (!objs.empty() && objs.back()->get_id() == -1)
		objs.pop_back();
	preview_circle.reset();
	canvas->update();
}


void EraserTool::erase_at(CanvasWidget* canvas, QPointF pos, int brush_thickness) {
	auto& objs = canvas->objects();
	std::erase_if(objs, [&](const auto& obj) {
		return obj->contains_point(pos, brush_thickness);
	});
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
