#include <QApplication>
#include <QUuid>

#include <AppController/AppController.h>
#include <UI/MainWindow.h>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	AppController controller;

	QUrl serverUrl("ws://localhost:8080");
	QString roomId = "default";
	if (argc > 1) {
		serverUrl = QUrl(argv[1]);
		QString path = serverUrl.path();
		if (path.startsWith('/') && path.length() > 1) {
			roomId = path.mid(1);
			serverUrl.setPath("");
		}
	}
	if (argc > 2) {
		roomId = QString::fromUtf8(argv[2]);
	}
	controller.connectToServer(serverUrl, roomId);

	controller.start();

	return QApplication::exec();
}
