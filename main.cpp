#include "emudiscer.h"
#include <QtWidgets/QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setApplicationName("EmuDiscer");
	app.setApplicationVersion("1.0.0");

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
	EmuDiscer w;

	if (parser.isSet(settingsOption))
		w.show();

	// finally, execute qt app
	return app.exec();
}
