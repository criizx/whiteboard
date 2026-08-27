#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QColor>
#include <QMainWindow>
#include <QToolButton>

class CanvasWidget;
class QToolBar;
class QAction;
class QSpinBox;
class QLabel;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr, const QString& clientId = QString());
	~MainWindow() override;
	void set_connection_status(const QString& text, bool connected);
	void update_object_count();

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
	void update_zoom_label(qreal scale);

private:
	CanvasWidget* canvas;
	QToolBar* toolbar{};
	QColor current_color = Qt::black;
	int current_thickness = 2;
	QToolButton* zoom_label{};
	QLabel* connection_label{};
	QLabel* object_count_label{};
	QToolButton* color_button{};

	QToolButton* setup_file_button();
	QToolButton* setup_session_button();
	void setup_toolbar();
	void setup_status_bar();
	void update_color_button();

signals:
	void createSessionRequested();
	void joinSessionRequested(const QString& connectionString);
	void copyLinkRequested();
};

#endif // MAINWINDOW_H
