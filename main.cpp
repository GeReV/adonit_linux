#include <csignal>

#include <QtCore/QCoreApplication>
#include <gatocentralmanager.h>

#include "tester.h"

static void handle_signal(int signum)
{
	Q_UNUSED(signum);
	QCoreApplication::postEvent(QCoreApplication::instance(),
	                            new QEvent(QEvent::Quit));
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	QScopedPointer<Tester> tester(new Tester);

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	tester->test();

	return a.exec();
}
