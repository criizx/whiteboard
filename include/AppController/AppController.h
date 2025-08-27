#pragma once

#include <memory>
#include <QObject>

#include <DrawingLogic/CanvasWidget.h>
#include <io/Delta_CRDT/WhiteboardSession.h>
#include <Shared/Shared.h>
#include <UI/MainWindow.h>

class AppController : public QObject {
    Q_OBJECT

private:
    QString m_clientId;
    WhiteboardSession* m_session;
    std::unique_ptr<MainWindow> m_mainWindow;
    CanvasWidget* m_canvasWidget;

    QString generateClientId();
    void setupConnections();

private slots:

    void onLocalObjectCreated(std::shared_ptr<DrawableObject> obj);
    void onLocalObjectModified(std::shared_ptr<DrawableObject> obj);
    void onLocalObjectDeleted(std::shared_ptr<DrawableObject> obj);
    void onLocalAllObjectsDeleted();

    void onRemoteObjectsUpdated(const QVector<DrawableObjectData>& objects);

public:
    AppController(QObject* parent = nullptr);
    ~AppController() override;

    void start();

};