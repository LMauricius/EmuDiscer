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

    // single instance
    QSharedMemory sharedMemory;
    sharedMemory.setKey (QString("EmuDiscerSharedData_")+getUsername());
    qDebug() << sharedMemory.nativeKey() << "\n";
    if (!sharedMemory.create(sizeof(SharedMemData)))
    {

        qDebug() << "Already running" << "\n";
        sharedMemory.attach();

        sharedMemory.lock();
        ((SharedMemData*)sharedMemory.data())->raiseWindow = true;
/*#ifdef _WIN32
        //SetForegroundWindow((HWND)(*((WId*)sharedMemory.data())));
        ShowWindow((HWND)(((SharedMemData*)sharedMemory.data())->mainWindow), SW_SHOWNORMAL);
#elif defined __unix__
#endif*/
        sharedMemory.unlock();

        sharedMemory.detach();
        return 0;

    }

	// command line
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption settingsOption(
		{ "s", "open-settings" },
		"Open settings window at launch"
	);
	parser.addOption(settingsOption);

	parser.process(app);

	// options window
    EmuDiscer w(&sharedMemory);

	if (parser.isSet(settingsOption))
		w.show();

    // save handle
    sharedMemory.attach();

    sharedMemory.lock();
    ((SharedMemData*)sharedMemory.data())->raiseWindow = false;
    //((SharedMemData*)sharedMemory.data())->mainWindow = w.winId();
    sharedMemory.unlock();

	// finally, execute qt app
	return app.exec();
}
