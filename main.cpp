#include <QApplication>
#include <QUuid>

#include <AppController/AppController.h>
#include <UI/MainWindow.h>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	AppController controller;
	controller.start();

	return QApplication::exec();
}
