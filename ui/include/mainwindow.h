#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QColor>
#include <QToolButton>

class CanvasWidget;
class QToolBar;
class QAction;
class QSpinBox;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void edit(){};
	void serialize(){};
	void deserialize(){};

	void select();
	void choose_color();
	void change_thickness(int value);
	void select_tool_line();
	void select_tool_rectangle();
	void select_tool_brush();
	void select_tool_eraser();
	void clear_canvas();

private:
	CanvasWidget* canvas;
	QToolBar* toolbar{};
	QColor current_color = Qt::black;
	int current_thickness = 2;

	QToolButton* setup_edit_button();
	QToolButton* setup_tools_button();
	void setup_toolbar();
};

#endif // MAINWINDOW_H
