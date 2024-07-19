#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	a.setOrganizationName("KeAr");
	a.setApplicationName(applicationName);
	MainWindow w;
	w.show();
	return a.exec();
}
