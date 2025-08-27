#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QMenu>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QMessageBox>

#include <DrawingLogic/CanvasWidget.h>
#include <DrawingLogic/Drawer.h>
#include <UI/MainWindow.h>
#include <io/Serialization/Serialization.h>

MainWindow::MainWindow(QWidget *parent,const QString& clientId)
    : QMainWindow(parent)
{
    setWindowTitle("Whiteboard");
    canvas = new CanvasWidget(this, clientId);
    setCentralWidget(canvas);
    setup_toolbar();
}

MainWindow::~MainWindow() = default;

void MainWindow::setup_toolbar() {
    toolbar = addToolBar("Tools");
    toolbar->setMovable(false);

    toolbar->addWidget(setup_file_button());
    toolbar->addWidget(setup_tools_button());

    auto* color_action = new QAction("Color", this);
    connect(color_action, &QAction::triggered, this, &MainWindow::choose_color);
    toolbar->addAction(color_action);

    auto* clear_action = new QAction("Clear", this);
    connect(clear_action, &QAction::triggered, this, &MainWindow::clear_canvas);
    toolbar->addAction(clear_action);

    auto* thickness_spin = new QSpinBox(this);
    thickness_spin->setRange(1, 50);
    thickness_spin->setValue(current_thickness);
    connect(thickness_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::change_thickness);
    toolbar->addWidget(thickness_spin);
}

QToolButton* MainWindow::setup_file_button() {
    auto* file_button = new QToolButton(this);
    file_button->setText("File");
    file_button->setPopupMode(QToolButton::InstantPopup);

    QMenu* file_menu = new QMenu(file_button);

    QAction* save_as_action = file_menu->addAction("Save as...");
    QAction* upload_action = file_menu->addAction("Upload");

    connect(save_as_action, &QAction::triggered, this, &MainWindow::save_as);
    connect(upload_action, &QAction::triggered, this, &MainWindow::upload);

    file_button->setMenu(file_menu);
    return file_button;
}

QToolButton* MainWindow::setup_tools_button() {
    auto* tool_button = new QToolButton(this);
    tool_button->setText("Tools");
    tool_button->setPopupMode(QToolButton::InstantPopup);

    QMenu *tool_menu = new QMenu(tool_button);

    auto addToolAction = [&](const QString &name, void (MainWindow::*slot)()) {
        QAction *action = tool_menu->addAction(name);
        connect(action, &QAction::triggered, this, slot);
    };

    addToolAction("Select", &MainWindow::select);
    addToolAction("Line", &MainWindow::select_tool_line);
    addToolAction("Rectangle", &MainWindow::select_tool_rectangle);
    addToolAction("Brush", &MainWindow::select_tool_brush);
    addToolAction("Eraser", &MainWindow::select_tool_eraser);

    tool_button->setMenu(tool_menu);
    return tool_button;
}

void MainWindow::save_as() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        "Save Whiteboard",
        "",
        "Whiteboard Files (*.wb);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        if (CanvasSerializer::serialize(canvas, filename)) {
            QMessageBox::information(this, "Success", "File saved successfully");
        }
        else {
            QMessageBox::warning(this, "Error", "Failed to save file");
        }
    }
}

void MainWindow::upload() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Load Whiteboard",
        "",
        "Whiteboard Files (*.wb);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        if (CanvasSerializer::deserialize(canvas, filename)) {
            QMessageBox::information(this, "Success", "File loaded successfully");
            canvas->update();
        }
        else {
            QMessageBox::warning(this, "Error", "Failed to load file");
        }
    }
}


void MainWindow::select() {
    auto tool = std::make_unique<MoveTool>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
}

void MainWindow::choose_color() {
    QColor selected = QColorDialog::getColor(current_color, this);
    if (selected.isValid()) {
        current_color = selected;
        canvas->set_pen_color(current_color);
    }
}

void MainWindow::clear_canvas() {
    canvas->clear_all();
}

void MainWindow::select_tool_line() {
    auto tool = std::make_unique<LineDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
}

void MainWindow::select_tool_rectangle() {
    auto tool = std::make_unique<RectangleDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
}

void MainWindow::select_tool_brush() {
    auto tool = std::make_unique<BrokenLineDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
}

void MainWindow::select_tool_eraser() {
    auto tool = std::make_unique<EraserTool>();
    tool->set_thickness(current_thickness);
    canvas->set_drawer(std::move(tool));
}

void MainWindow::change_thickness(int value) {
    current_thickness = value;
    if (canvas) {
        canvas->set_pen_thickness(current_thickness);
    }
}