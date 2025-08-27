#include <QUuid>

#include <AppController/AppController.h>
#include <DrawingLogic/CanvasWidget.h>

QString AppController::generateClientId(){
	return QUuid::createUuid().toString(QUuid::WithoutBraces);
}


AppController::AppController(QObject* parent)
    : QObject(parent),
    m_session(new WhiteboardSession(this))
{
    m_clientId = generateClientId();
    m_mainWindow = std::make_unique<MainWindow>(nullptr, m_clientId);
    m_canvasWidget = m_mainWindow->getCanvas();


    setupConnections();
}

void AppController::setupConnections() {
    connect(m_canvasWidget, &CanvasWidget::objectCreated, this, &AppController::onLocalObjectCreated);
    connect(m_canvasWidget, &CanvasWidget::objectModified, this, &AppController::onLocalObjectModified);
    connect(m_canvasWidget, &CanvasWidget::objectDeleted, this, &AppController::onLocalObjectDeleted);
    connect(m_canvasWidget, &CanvasWidget::allObjectsDeleted, this, &AppController::onLocalAllObjectsDeleted);

    connect(m_session, &WhiteboardSession::objectsUpdated, this, &AppController::onRemoteObjectsUpdated);
}

void AppController::start() {
    if (m_mainWindow) {
        m_mainWindow->showFullScreen();
    }
}

AppController::~AppController() {
    if (m_mainWindow) {
        if (m_mainWindow->isVisible()) m_mainWindow->close();
        m_mainWindow = nullptr;
    }
}

void AppController::onLocalObjectCreated(std::shared_ptr<DrawableObject> obj) {
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalCreate(data);
}

void AppController::onRemoteObjectsUpdated(const QVector<DrawableObjectData>& objects) {
    if (!m_canvasWidget) {
        qWarning() << "CanvasWidget is null in onRemoteObjectsUpdated";
        return;
    }
    std::vector<std::shared_ptr<DrawableObject>> uiObjects;
    uiObjects.reserve(objects.size());

    for (const auto& data : objects) {
        try {
            auto obj = DrawableObject::fromDrawableObjectData(data);
            if (obj) {
                uiObjects.push_back(obj);
            }
        }
        catch (const std::exception& e) {
            qWarning() << "Failed to convert DrawableObjectData to DrawableObject:" << e.what();
        }
    }
}

void AppController::onLocalObjectModified(std::shared_ptr<DrawableObject> obj) {
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalModify(data);
}

void AppController::onLocalObjectDeleted(std::shared_ptr<DrawableObject> obj) {
    DrawableObjectData data = obj->toDrawableObjectData();
    m_session->onLocalDelete(data);
}