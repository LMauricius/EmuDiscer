/*#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#elif defined __unix__
#endif*/

#include "emudiscer.h"
#include "Utilities.h"
#include <QtWidgets/QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
	app.setApplicationName("EmuDiscer");
    app.setApplicationVersion(EMUDISCER_APP_VERSION);

	// command line
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption settingsOption(
		{ "s", "open-settings" },
		"Open settings window at launch"
	);
	parser.addOption(settingsOption);
    QCommandLineOption multiInstanceOption(
        { "m", "multi-instance" },
        "Run program even if another instance is running"
    );
    parser.addOption(multiInstanceOption);

	parser.process(app);

    // single instance
    QSharedMemory sharedMemory;
    sharedMemory.setKey (QString("EmuDiscerSharedData_")+getUsername());
    qDebug() << sharedMemory.nativeKey() << "\n";
    if (!sharedMemory.create(sizeof(SharedMemData)) && !parser.isSet(multiInstanceOption))
    {

        qDebug() << "Already running" << "\n";
        sharedMemory.attach();

        sharedMemory.lock();
        ((SharedMemData*)sharedMemory.data())->raiseWindow = true;
        sharedMemory.unlock();

        sharedMemory.detach();
        return 0;

    }

	// options window
    EmuDiscer w(&sharedMemory);

	if (parser.isSet(settingsOption))
		w.show();

    // attach to shared data
    sharedMemory.attach();

    sharedMemory.lock();
    ((SharedMemData*)sharedMemory.data())->raiseWindow = false;
    sharedMemory.unlock();

	// finally, execute qt app
	return app.exec();
}
