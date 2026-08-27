#include <QUuid>
#include <QDebug>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>

#include <AppController/AppController.h>
#include <DrawingLogic/CanvasWidget.h>

QString AppController::generateClientId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

AppController::AppController(QObject* parent)
    : QObject(parent),
      m_session(new WhiteboardSession(this))
{
    m_clientId  = generateClientId();
    m_mainWindow = std::make_unique<MainWindow>(nullptr, m_clientId);
    m_canvasWidget = m_mainWindow->getCanvas();

    setupConnections();
}

AppController::~AppController()
{
    if (m_mainWindow) {
        if (m_mainWindow->isVisible()) m_mainWindow->close();
        m_mainWindow = nullptr;
    }
}

void AppController::start()
{
    if (m_mainWindow) {
        m_mainWindow->showMaximized();
    }
}

void AppController::connectToServer(const QUrl& serverUrl, const QString& roomId)
{
    const QString room = roomId.isEmpty() ? QStringLiteral("default") : roomId;
    m_serverUrl = serverUrl;
    m_roomId = room;
    qDebug() << "[AppController] Connecting to server:" << serverUrl << "room:" << room;
    m_session->disconnectFromServer();
    m_mainWindow->set_connection_status("Connecting…", false);
    m_session->connectToServer(serverUrl, room, m_clientId);
}

void AppController::setupConnections()
{
    connect(m_canvasWidget, &CanvasWidget::objectCreated,
            this, &AppController::onLocalObjectCreated);
    connect(m_canvasWidget, &CanvasWidget::objectModified,
            this, &AppController::onLocalObjectModified);
    connect(m_canvasWidget, &CanvasWidget::objectDeleted,
            this, &AppController::onLocalObjectDeleted);
    connect(m_canvasWidget, &CanvasWidget::allObjectsDeleted,
            this, &AppController::onLocalAllObjectsDeleted);

    connect(m_session, &WhiteboardSession::objectsUpdated,
            this, &AppController::onRemoteObjectsUpdated);

    connect(m_session, &WhiteboardSession::networkConnected,
            this, &AppController::onNetworkConnected);
    connect(m_session, &WhiteboardSession::networkDisconnected,
            this, &AppController::onNetworkDisconnected);
    connect(m_session, &WhiteboardSession::networkError,
            this, &AppController::onNetworkError);
    connect(m_session, &WhiteboardSession::peerCountChanged, this, [this](int count) {
        if (count > 0) {
            m_mainWindow->set_connection_status(
                QString("Direct · %1 peer%2").arg(count).arg(count == 1 ? "" : "s"), true);
        } else if (m_session->isOnline()) {
            m_mainWindow->set_connection_status("Connected", true);
        }
    });

    connect(m_mainWindow.get(), &MainWindow::createSessionRequested,
            this, &AppController::onCreateSessionRequested);
    connect(m_mainWindow.get(), &MainWindow::joinSessionRequested,
            this, &AppController::onJoinSessionRequested);
    connect(m_mainWindow.get(), &MainWindow::copyLinkRequested,
            this, &AppController::onCopyLinkRequested);
}

void AppController::onLocalObjectCreated(std::shared_ptr<DrawableObject> obj)
{
    if (!obj) return;
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalCreate(data);
}

void AppController::onLocalObjectModified(std::shared_ptr<DrawableObject> obj)
{
    if (!obj) return;
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalModify(data);
}

void AppController::onLocalObjectDeleted(std::shared_ptr<DrawableObject> obj)
{
    if (!obj) return;
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalDelete(data);
}

void AppController::onLocalAllObjectsDeleted()
{
    m_session->onLocalDeleteAll();
}

void AppController::onRemoteObjectsUpdated(const QVector<DrawableObjectData>& objects)
{
    if (!m_canvasWidget) {
        qWarning() << "[AppController] CanvasWidget is null in onRemoteObjectsUpdated";
        return;
    }

    m_canvasWidget->blockSignals(true);
    m_canvasWidget->clear_all();

    for (const auto& data : objects) {
        try {
            auto obj = DrawableObject::fromDrawableObjectData(data);
            if (obj) {
                m_canvasWidget->addObject(obj);
            }
        } catch (const std::exception& e) {
            qWarning() << "[AppController] Failed to reconstruct object:"
                       << e.what() << "id:" << data.id;
        }
    }

    m_canvasWidget->blockSignals(false);
    m_canvasWidget->update();
    m_mainWindow->update_object_count();
}

void AppController::onNetworkConnected()
{
    qDebug() << "[AppController] Network connected";
    m_mainWindow->set_connection_status("Connected", true);
}

void AppController::onNetworkDisconnected()
{
    qDebug() << "[AppController] Network disconnected";
    m_mainWindow->set_connection_status("Offline", false);
}

void AppController::onNetworkError(const QString& message)
{
    qWarning() << "[AppController] Network error:" << message;
    m_mainWindow->set_connection_status("Connection error", false);
}

void AppController::onCreateSessionRequested()
{
    QString newRoomId = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
    connectToServer(m_serverUrl, newRoomId);
    
    QUrl linkUrl = m_serverUrl;
    linkUrl.setPath("/" + newRoomId);
    QGuiApplication::clipboard()->setText(linkUrl.toString());
    
    QMessageBox::information(m_mainWindow.get(), "New Session Created",
                             "A new collaborative session has been created.\n\n"
                             "The connection link has been copied to your clipboard:\n" + linkUrl.toString());
}

void AppController::onJoinSessionRequested(const QString& connectionString)
{
    if (connectionString.isEmpty()) return;
    
    QUrl url(connectionString.trimmed());
    if (!url.isValid()) {
        QMessageBox::warning(m_mainWindow.get(), "Error", "Invalid link/URL entered.");
        return;
    }
    
    QUrl serverUrl = url;
    QString roomId = "default";
    
    QString path = url.path();
    if (path.startsWith('/') && path.length() > 1) {
        roomId = path.mid(1);
        serverUrl.setPath("");
    }
    
    connectToServer(serverUrl, roomId);
    QMessageBox::information(m_mainWindow.get(), "Connecting",
                             QString("Connecting to server: %1\nRoom: %2").arg(serverUrl.toString()).arg(roomId));
}

void AppController::onCopyLinkRequested()
{
    QUrl linkUrl = m_serverUrl;
    linkUrl.setPath("/" + m_roomId);
    QGuiApplication::clipboard()->setText(linkUrl.toString());
    
    QMessageBox::information(m_mainWindow.get(), "Link Copied",
                             "The current room link has been copied to your clipboard:\n" + linkUrl.toString());
}
