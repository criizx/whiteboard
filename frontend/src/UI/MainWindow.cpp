#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QMenu>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QLabel>
#include <QKeySequence>
#include <QStatusBar>
#include <QActionGroup>
#include <QSizePolicy>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

#include <DrawingLogic/CanvasWidget.h>
#include <DrawingLogic/Drawer.h>
#include <UI/MainWindow.h>
#include <io/Serialization/Serialization.h>

namespace {
QIcon tool_icon(const QString& name) {
    QPixmap pixmap(28, 28);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#344054"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    if (name == "select") {
        QPainterPath path;
        path.moveTo(7, 4); path.lineTo(20, 17); path.lineTo(14, 18);
        path.lineTo(11, 24); path.closeSubpath();
        painter.drawPath(path);
    } else if (name == "brush") {
        QPainterPath path;
        path.moveTo(5, 21); path.cubicTo(9, 8, 15, 23, 23, 6);
        painter.drawPath(path);
    } else if (name == "line") {
        painter.drawLine(QPointF(5, 22), QPointF(23, 6));
    } else if (name == "rectangle") {
        painter.drawRoundedRect(QRectF(5, 6, 18, 16), 2, 2);
    } else if (name == "eraser") {
        painter.drawPolygon(QPolygonF({{7, 19}, {16, 7}, {23, 14}, {14, 23}}));
    } else if (name == "grid") {
        for (int pos : {7, 14, 21}) {
            painter.drawLine(pos, 5, pos, 23);
            painter.drawLine(5, pos, 23, pos);
        }
    }
    return QIcon(pixmap);
}
}

MainWindow::MainWindow(QWidget *parent,const QString& clientId)
    : QMainWindow(parent)
{
    setWindowTitle("Whiteboard — Collaborative canvas");
    resize(1280, 800);
    canvas = new CanvasWidget(this, clientId);
    setCentralWidget(canvas);
    setup_toolbar();
    setup_status_bar();

    setStyleSheet(R"(
        QMainWindow { background: #f5f6f8; }
        CanvasWidget#canvas { background: #fbfbfc; }
        QToolBar#headerBar {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #dfe3e8;
            spacing: 4px;
            padding: 8px 14px;
        }
        QToolBar#toolRail {
            background: #ffffff;
            border: none;
            border-right: 1px solid #dfe3e8;
            spacing: 5px;
            padding: 10px 7px;
        }
        QToolBar QToolButton {
            border: 1px solid transparent;
            border-radius: 9px;
            padding: 7px 10px;
            color: #273142;
        }
        QToolBar QToolButton:hover { background: #f0f3f7; }
        QToolButton#dangerButton { color: #b42318; }
        QToolButton#dangerButton:hover { background: #fef3f2; }
        QToolBar QToolButton:checked {
            color: #155eef;
            background: #eaf1ff;
            border-color: #bfd1ff;
        }
        QToolBar#toolRail QToolButton {
            min-width: 66px;
            padding: 8px 5px;
        }
        QToolBar QSpinBox {
            border: 1px solid #d7dce2;
            border-radius: 7px;
            padding: 5px 7px;
            background: #ffffff;
        }
        QStatusBar {
            background: #ffffff;
            border-top: 1px solid #e3e6ea;
            color: #667085;
        }
        QStatusBar QToolButton {
            border: none;
            border-radius: 5px;
            padding: 3px 7px;
            color: #344054;
        }
        QStatusBar QToolButton:hover { background: #eef2f6; }
    )");

    statusBar()->showMessage("Line tool · drag to draw · middle mouse to pan", 5000);
    connect(canvas, &CanvasWidget::viewChanged, this, &MainWindow::update_zoom_label);
    connect(canvas, &CanvasWidget::objectCreated, this, [this] { update_object_count(); });
    connect(canvas, &CanvasWidget::objectDeleted, this, [this] { update_object_count(); });
    connect(canvas, &CanvasWidget::allObjectsDeleted, this, [this] { update_object_count(); });
}

MainWindow::~MainWindow() = default;

void MainWindow::setup_toolbar() {
    auto* header = new QToolBar("Project", this);
    header->setObjectName("headerBar");
    header->setMovable(false);
    header->setFloatable(false);
    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addToolBar(Qt::TopToolBarArea, header);

    auto* brand = new QLabel(
        "<span style='font-size:17px;font-weight:700;color:#101828'>Whiteboard</span>"
        "<br><span style='font-size:10px;color:#98a2b3'>Collaborative canvas</span>", this);
    brand->setMinimumWidth(155);
    header->addWidget(brand);
    header->addSeparator();
    header->addWidget(setup_file_button());
    header->addWidget(setup_session_button());

    auto* header_spacer = new QWidget(this);
    header_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    header->addWidget(header_spacer);

    color_button = new QToolButton(this);
    color_button->setText("Color");
    color_button->setToolTip("Choose stroke color");
    color_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(color_button, &QToolButton::clicked, this, &MainWindow::choose_color);
    header->addWidget(color_button);
    update_color_button();

    auto* width_label = new QLabel("Width", this);
    width_label->setStyleSheet("color: #667085; padding-left: 8px;");
    header->addWidget(width_label);

    auto* thickness_spin = new QSpinBox(this);
    thickness_spin->setRange(1, 50);
    thickness_spin->setSuffix(" px");
    thickness_spin->setValue(current_thickness);
    thickness_spin->setToolTip("Stroke width");
    connect(thickness_spin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::change_thickness);
    header->addWidget(thickness_spin);

    header->addSeparator();
    auto* clear_button = new QToolButton(this);
    clear_button->setObjectName("dangerButton");
    clear_button->setText("Clear");
    clear_button->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backspace));
    clear_button->setToolTip("Clear canvas · Ctrl+Backspace");
    connect(clear_button, &QToolButton::clicked, this, &MainWindow::clear_canvas);
    header->addWidget(clear_button);

    toolbar = new QToolBar("Drawing tools", this);
    toolbar->setObjectName("toolRail");
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setIconSize(QSize(26, 26));
    toolbar->setMinimumWidth(88);
    addToolBar(Qt::LeftToolBarArea, toolbar);

    auto* tool_group = new QActionGroup(this);
    tool_group->setExclusive(true);
    auto add_tool = [this, tool_group](const QString& text, const QString& icon_name,
                                      const QKeySequence& shortcut,
                                      const QString& tip, void (MainWindow::*slot)()) {
        auto* action = toolbar->addAction(tool_icon(icon_name), text);
        action->setCheckable(true);
        action->setShortcut(shortcut);
        action->setToolTip(tip);
        tool_group->addAction(action);
        connect(action, &QAction::triggered, this, slot);
        return action;
    };

    add_tool("Select", "select", QKeySequence(Qt::Key_V), "Select and move · V", &MainWindow::select);
    add_tool("Brush", "brush", QKeySequence(Qt::Key_B), "Freehand brush · B", &MainWindow::select_tool_brush);
    auto* line_action = add_tool("Line", "line", QKeySequence(Qt::Key_L), "Straight line · L", &MainWindow::select_tool_line);
    add_tool("Rectangle", "rectangle", QKeySequence(Qt::Key_R), "Rectangle · R", &MainWindow::select_tool_rectangle);
    add_tool("Eraser", "eraser", QKeySequence(Qt::Key_E), "Eraser · E", &MainWindow::select_tool_eraser);
    line_action->setChecked(true);

    toolbar->addSeparator();
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);

    auto* grid_action = toolbar->addAction(tool_icon("grid"), "Grid");
    grid_action->setCheckable(true);
    grid_action->setChecked(true);
    grid_action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
    grid_action->setToolTip("Toggle grid · Ctrl+G");
    connect(grid_action, &QAction::toggled, canvas, &CanvasWidget::set_grid_visible);
}

void MainWindow::setup_status_bar() {
    auto* zoom_out_button = new QToolButton(this);
    zoom_out_button->setText("−");
    zoom_out_button->setShortcut(QKeySequence::ZoomOut);
    zoom_out_button->setToolTip("Zoom out · Ctrl+-");
    connect(zoom_out_button, &QToolButton::clicked, canvas, &CanvasWidget::zoom_out);
    statusBar()->addPermanentWidget(zoom_out_button);

    zoom_label = new QToolButton(this);
    zoom_label->setText("100%");
    zoom_label->setMinimumWidth(52);
    zoom_label->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    zoom_label->setToolTip("Reset view · Ctrl+0");
    connect(zoom_label, &QToolButton::clicked, canvas, &CanvasWidget::reset_view);
    statusBar()->addPermanentWidget(zoom_label);

    auto* zoom_in_button = new QToolButton(this);
    zoom_in_button->setText("+");
    zoom_in_button->setShortcut(QKeySequence::ZoomIn);
    zoom_in_button->setToolTip("Zoom in · Ctrl++");
    connect(zoom_in_button, &QToolButton::clicked, canvas, &CanvasWidget::zoom_in);
    statusBar()->addPermanentWidget(zoom_in_button);

    object_count_label = new QLabel(this);
    object_count_label->setMinimumWidth(82);
    object_count_label->setAlignment(Qt::AlignCenter);
    statusBar()->addPermanentWidget(object_count_label);

    connection_label = new QLabel(this);
    connection_label->setMinimumWidth(112);
    connection_label->setAlignment(Qt::AlignCenter);
    statusBar()->addPermanentWidget(connection_label);

    update_object_count();
    set_connection_status("Connecting…", false);
}

void MainWindow::update_color_button() {
    if (!color_button) return;
    QPixmap swatch(20, 20);
    swatch.fill(Qt::transparent);
    QPainter painter(&swatch);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#d0d5dd"), 1));
    painter.setBrush(current_color);
    painter.drawEllipse(QRectF(2, 2, 16, 16));
    color_button->setIcon(QIcon(swatch));
}

void MainWindow::set_connection_status(const QString& text, bool connected) {
    if (!connection_label) return;
    connection_label->setText(QString("● %1").arg(text));
    const QString color = connected ? "#079455" :
        (text.startsWith("Connecting") ? "#667085" : "#d92d20");
    connection_label->setStyleSheet(QString("color: %1; padding: 0 8px;")
        .arg(color));
}

void MainWindow::update_object_count() {
    if (!object_count_label || !canvas) return;
    const qsizetype count = static_cast<qsizetype>(canvas->objects().size());
    object_count_label->setText(QString("%1 object%2").arg(count).arg(count == 1 ? "" : "s"));
}

QToolButton* MainWindow::setup_file_button() {
    auto* file_button = new QToolButton(this);
    file_button->setText("File");
    file_button->setPopupMode(QToolButton::InstantPopup);

    QMenu* file_menu = new QMenu(file_button);

    QAction* save_as_action = file_menu->addAction("Save as...");
    QAction* upload_action = file_menu->addAction("Upload");

    save_as_action->setShortcut(QKeySequence::SaveAs);
    upload_action->setShortcut(QKeySequence::Open);

    connect(save_as_action, &QAction::triggered, this, &MainWindow::save_as);
    connect(upload_action, &QAction::triggered, this, &MainWindow::upload);

    file_button->setMenu(file_menu);
    return file_button;
}

QToolButton* MainWindow::setup_session_button() {
    auto* session_button = new QToolButton(this);
    session_button->setText("Session");
    session_button->setPopupMode(QToolButton::InstantPopup);

    QMenu* session_menu = new QMenu(session_button);

    QAction* create_action = session_menu->addAction("Create New Session");
    QAction* join_action = session_menu->addAction("Join Session...");
    QAction* copy_action = session_menu->addAction("Copy Room Link");

    connect(create_action, &QAction::triggered, this, &MainWindow::createSessionRequested);
    connect(join_action, &QAction::triggered, this, [this]() {
        bool ok;
        QString text = QInputDialog::getText(this, "Join Session",
                                             "Enter Room Link or Code (e.g. ws://localhost:8080/room-id):",
                                             QLineEdit::Normal, "", &ok);
        if (ok && !text.trimmed().isEmpty()) {
            emit joinSessionRequested(text.trimmed());
        }
    });
    connect(copy_action, &QAction::triggered, this, &MainWindow::copyLinkRequested);

    session_button->setMenu(session_menu);
    return session_button;
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
    canvas->set_idle_cursor(Qt::ArrowCursor);
    statusBar()->showMessage("Select tool (V)");
}

void MainWindow::choose_color() {
    QColor selected = QColorDialog::getColor(current_color, this);
    if (selected.isValid()) {
        current_color = selected;
        canvas->set_pen_color(current_color);
        update_color_button();
    }
}

void MainWindow::clear_canvas() {
    if (canvas->objects().empty()) return;
    const auto answer = QMessageBox::question(
        this, "Clear canvas", "Remove every object from the canvas?",
        QMessageBox::Cancel | QMessageBox::Yes, QMessageBox::Cancel);
    if (answer == QMessageBox::Yes) canvas->clear_all();
}

void MainWindow::select_tool_line() {
    auto tool = std::make_unique<LineDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
    canvas->set_idle_cursor(Qt::CrossCursor);
    statusBar()->showMessage("Line tool (L)");
}

void MainWindow::select_tool_rectangle() {
    auto tool = std::make_unique<RectangleDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
    canvas->set_idle_cursor(Qt::CrossCursor);
    statusBar()->showMessage("Rectangle tool (R)");
}

void MainWindow::select_tool_brush() {
    auto tool = std::make_unique<BrokenLineDrawer>();
    tool->set_thickness(current_thickness);
    tool->set_color(current_color);
    canvas->set_drawer(std::move(tool));
    canvas->set_idle_cursor(Qt::CrossCursor);
    statusBar()->showMessage("Brush tool (B)");
}

void MainWindow::select_tool_eraser() {
    auto tool = std::make_unique<EraserTool>();
    tool->set_thickness(current_thickness);
    canvas->set_drawer(std::move(tool));
    canvas->set_idle_cursor(Qt::CrossCursor);
    statusBar()->showMessage("Eraser tool (E)");
}

void MainWindow::update_zoom_label(qreal scale) {
    if (zoom_label) {
        zoom_label->setText(QString::number(qRound(scale * 100.0)) + "%");
    }
}

void MainWindow::change_thickness(int value) {
    current_thickness = value;
    if (canvas) {
        canvas->set_pen_thickness(current_thickness);
    }
}
