#pragma once

#include <memory>
#include <QPointF>
#include <utility>
#include <vector>

#include <DrawingLogic/DrawableObject.h>

class CanvasWidget;
class DrawableObject;

class Drawer {
protected:
	int thickness = 3;
	QColor color = Qt::black;
	QBrush fill = Qt::NoBrush;

public:
	virtual ~Drawer() = default;

	void set_thickness(const int value) { thickness = value; }
	void set_color(QColor value) { color = std::move(value); }
	void set_fill(QBrush value) { fill = std::move(value); }

	[[nodiscard]] int get_thickness() const { return thickness; }
	[[nodiscard]] QColor get_color() const { return color; }
	[[nodiscard]] QBrush get_fill() const { return fill; }

	virtual void on_mouse_press(CanvasWidget* canvas, QPointF pos) = 0;
	virtual void on_mouse_move(CanvasWidget* canvas, QPointF pos) = 0;
	virtual void on_mouse_release(CanvasWidget* canvas, QPointF pos) = 0;
};


class BrokenLineDrawer : public Drawer {
public:
    void on_mouse_press(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_move(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_release(CanvasWidget* canvas, QPointF pos) override;

private:
    QVector<QPointF> m_points;
    bool m_drawing = false;
	std::shared_ptr<DrawableBrokenLine> preview_path;

};

class LineDrawer : public Drawer {
public:
    void on_mouse_press(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_move(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_release(CanvasWidget* canvas, QPointF pos) override;

private:
    bool m_drawing = false;
    QPointF start_point;
    QPointF end_point;

    std::shared_ptr<DrawableLine> preview_line;
};

class RectangleDrawer : public Drawer {
public:
    void on_mouse_press(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_move(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_release(CanvasWidget* canvas, QPointF pos) override;

private:
    bool m_drawing = false;
    QPointF start_point;
    QPointF end_point;

    std::shared_ptr<DrawableRectangle> preview_rectangle;
};

class EraserTool final : public Drawer {
public:
    void on_mouse_press(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_move(CanvasWidget* canvas, QPointF pos) override;
    void on_mouse_release(CanvasWidget* canvas, QPointF pos) override;

private:
    static void erase_at(CanvasWidget* canvas, QPointF pos, int brush_thickness) ;
	bool is_erasing = false;

	QPointF center;
	std::shared_ptr<DrawableAssistCircle> preview_circle;
};

class MoveTool final : public Drawer {
	public:
    void on_mouse_press(CanvasWidget* canvas, QPointF pos) override;
	void on_mouse_move(CanvasWidget* canvas, QPointF pos) override;
	void on_mouse_release(CanvasWidget* canvas, QPointF pos) override;
private:
	std::shared_ptr<DrawableObject> selected;
	QPointF last_pos;
};