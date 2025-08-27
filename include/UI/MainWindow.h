#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QToolButton>

class CanvasWidget;
class QToolBar;
class QAction;
class QSpinBox;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr, const QString& clientId = QString());
	~MainWindow() override;

	[[nodiscard]] CanvasWidget* getCanvas() {
		return canvas;
	}

private slots:
	void save_as();
	void upload();

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

	QToolButton* setup_file_button();
	QToolButton* setup_tools_button();
	void setup_toolbar();
};

#endif // MAINWINDOW_H
