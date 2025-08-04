#include "../include/mainwindow.h"
#include "../../drawing_logic/include/canvaswidget.h"
#include "../../drawing_logic/include/drawer.h"
#include <QColorDialog>
#include <QSpinBox>
#include <QAction>
#include <QToolBar>
#include <QToolButton>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Whiteboard");
    canvas = new CanvasWidget(this);
    setCentralWidget(canvas);
    setup_toolbar();
}

MainWindow::~MainWindow() = default;

void MainWindow::setup_toolbar() {
    toolbar = addToolBar("Tools");
    toolbar->setMovable(false);

    toolbar->addWidget(setup_edit_button());
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

QToolButton* MainWindow::setup_edit_button() {
    auto* edit_button = new QToolButton(this);
    edit_button->setText("Edit");
    edit_button->setPopupMode(QToolButton::InstantPopup);

    QMenu *edit_menu = new QMenu(edit_button);

    QAction *serialize_action = edit_menu->addAction("Save as...");
    QAction *deserialize_action = edit_menu->addAction("Load from file");

    connect(serialize_action, &QAction::triggered, this, &MainWindow::serialize);
    connect(deserialize_action, &QAction::triggered, this, &MainWindow::deserialize);

    edit_button->setMenu(edit_menu);
    return edit_button;
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