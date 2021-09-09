#include "emudiscer.h"

#include "Utilities.h"
#include "EmulatorBuiltInOptions.h"
#include "appdialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QEvent>
#include <QStyle>
#include <QTabWidget>
#include <QTabBar>
#include <QEvent>
#include <QCloseEvent>
#include <QDirIterator>
#include <QDebug>
#include <QTextStream>

#ifdef WIN32
#include "windows.h"
#include "dbt.h"
#endif

#ifdef __unix__
#include <QtDBus>
#include "UDisks2Watcher.h"
#endif

#include <fstream>


EmuDiscer::EmuDiscer(QWidget *parent)
	: QDialog(parent)
    , mConfig((getConfigDirectory()+"/config.ini").toStdWString(), true)
{
    ui.setupUi(this);

    if (!std::filesystem::exists(getConfigDirectory().toStdWString()))
    {
        std::filesystem::create_directory(getConfigDirectory().toStdWString());
    }

	/* System tray icon*/
	QAction *settingsAction = new QAction(tr("Open"), this);
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showNormal()));
	QAction *quitAction = new QAction(tr("Exit (Middle Mouse)"), this);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	mSysTrayMenu = new QMenu(this); 
	mSysTrayMenu->addAction(settingsAction);
	mSysTrayMenu->addAction(quitAction);
	mSysTrayIcon = new QSystemTrayIcon(this);
	mSysTrayIcon->setContextMenu(mSysTrayMenu);
    mSysTrayIcon->setIcon(QIcon(windowIcon().pixmap(128)));
	mSysTrayIcon->setToolTip("EmuDiscer v1.0");
	connect(mSysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(on_trayIconActivated(QSystemTrayIcon::ActivationReason)));
	connect(mSysTrayIcon, SIGNAL(messageClicked()), this, SLOT(on_notificationClicked()));
	mSysTrayIcon->show();

	/*UI Setup*/
	connect(ui.okButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui.quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
	//ui.emulatorsTabWidget->currentWidget()->setLayout(ui.emulatorSettingsLayout);
    on_emulatorsTabWidget_currentChanged(ui.emulatorsTabWidget->currentIndex());
    connect(qApp, SIGNAL(saveStateRequest(QSessionManager&)), this, SLOT(on_saveStateRequest(QSessionManager&)));

    /*Macro list*/
    mMacrosMenu = new QMenu(this);
    for (
         QString macro : {
            "(DRIVE)",
            "(DIRECTORY)",
            "(BOOT_FILE)"
        }
    ) {
        QAction *action = new QAction(macro, this);
        //connect(action, SIGNAL(triggered()), this, SLOT(macroSelected()));
        mMacrosMenu->addAction(action);
    }
    //connect(ui.macrosButton, SIGNAL(triggered()), mMacrosMenu, SLOT(show()));

    // THIS IS NOT REALLY THE SAFEST WAY TO DETECT ALL NEW MOUNT POINTS
    /*Setup file watcher if needed*/
    /*auto driveDirs = getRemovableDriveDirectories();
    if (driveDirs.size() > 0)
    {
        mFileWatcher->addPaths(driveDirs);
        //connect(mFileWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(on_mediaDirChanged(const QString &)));
        for (const QString &path : driveDirs)
        {
            QDir dir(path);
            for (const QString &str : dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot)) {
                mMediaDirectories.push_back(path + "/" + str);
            }
        }
    }*/

#if defined __unix__
    // unix' way of reacting to drive change
    UDisks2Watcher *udisks2watcher = new UDisks2Watcher(this);
    connect(udisks2watcher, SIGNAL(partitionChanged(QString)), this, SLOT(on_partitionChanged(QString)));
    connect(udisks2watcher, SIGNAL(driveMounted(QString,QString)), this, SLOT(on_driveMounted(QString,QString)));
#endif

	/*
    Set defaults if needed
	*/
	mConfig.getStr(L"PS1", L"Path", L"");
    mConfig.getStr(L"PS1", L"Options", L"");
	mConfig.getStr(L"PS2", L"Path", L"");
    mConfig.getStr(L"PS2", L"Options", L"");
    mConfig.getStr(L"PSP", L"Path", L"");
    mConfig.getStr(L"PSP", L"Options", L"");
    mConfig.getStr(L"Gamecube", L"Path", L"");
    mConfig.getStr(L"Gamecube", L"Options", L"");
    mConfig.getStr(L"Wii", L"Path", L"");
    mConfig.getStr(L"Wii", L"Options", L"");

	/* Load config*/
	ui.autoRunCheckbox->setCheckState(
        mConfig.get<bool>(L"System", L"AutoRun", false) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked
    );
	ui.notificationsCheckbox->setCheckState(
		mConfig.get<bool>(L"System", L"ShowNotifications", true) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked
	);
	//ui.emulatorList->setCurrentItem(ui.emulatorList->itemAt(0, 0));

    // show to receive native events
    //qApp->installNativeEventFilter(this);
    // show & hide to receive events
    show();
    hide();
}

EmuDiscer::~EmuDiscer()
{
    //qApp->removeNativeEventFilter(this);
}

void EmuDiscer::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}

bool EmuDiscer::nativeEvent(const QByteArray & eventType, void * message, long *result)
{
#ifdef WIN32
	MSG* msg = (MSG*)message;

	if (msg->message == WM_DEVICECHANGE)
	{
		if (msg->wParam == DBT_DEVICEARRIVAL)
		{
			if (((PDEV_BROADCAST_HDR)msg->lParam)->dbch_devicetype == DBT_DEVTYP_VOLUME)
			{
				PDEV_BROADCAST_VOLUME pDBV = (PDEV_BROADCAST_VOLUME)msg->lParam;

				//if (pDBV->dbcv_flags & DBTF_MEDIA)
				{
                    char driveLetter = 'C';
					for (int i = 0; i < sizeof(DWORD) * 8; i++)
					{
						if (pDBV->dbcv_unitmask & (1 << i))
						{
							driveLetter = 'A' + i;// first letter is A
							break;
						}
					}

                    // load header bytes
                    char headerBuffer[128];
                    size_t charsToRead = 128;
                    charsToRead = getDriveFileHeader(QString(driveLetter), headerBuffer, charsToRead);

                    // make an iso from the drive file (was an experiment - not working)
                    /*std::string linkFilename = mTempDir.path().toStdString() + "driveIsoFile" + driveLetter + ".iso";
                    std::error_code ec;
                    std::filesystem::create_symlink(
                        driveFilename,
                        linkFilename,
                        ec
                    );
                    if (ec.value())
                    {
                        qCritical() << "Couldn't link file: " << QString::fromStdString(ec.message()) << "\n";
                    }*/

                    // launch Emu
                    if (
                        insertedMediaDir(
                            QString() + driveLetter + ":",
                            QString(driveLetter)
                        )
                    )
                    {
                    }
                    else if (
                        insertedMediaRaw(
                            QString(driveLetter),
                            headerBuffer,
                            charsToRead
                        )
                    )
                    {
                    }
				}
			}
		}
	}
#endif

    return QDialog::nativeEvent(eventType, message, result);// don't filter
}

bool EmuDiscer::tryLaunchEmu(QString emulatorType, QString mediaType, const std::map<std::wstring, std::wstring>& vars)
{
    if (emulatorType != "")
    {
        std::wstring path = mConfig.getStr(emulatorType.toStdWString(), L"Path");
        std::wstring options = mConfig.getStr(emulatorType.toStdWString(), L"Options");

        if (path != L"" && fileExists(QString::fromStdWString(path)))
        {
            for (auto& p : vars)
            {
                replaceAll(options, p.first, p.second);
            }

            startProgram(path, options);
        }
        else if (mConfig.get<bool>(L"System", L"ShowNotifications"))
        {
            mSysTrayIcon->showMessage("No emulator", QString("A %1 has been inserted but a %2 emulator has not been set up. Click to set it up.").arg(mediaType, emulatorType), windowIcon());
        }

        return true;
    }
    else
    {
        return false;
    }
}

bool EmuDiscer::insertedMediaDir(QString directory, QString drive)
{
	QString mediaType;
	QString emulatorType;
	std::map<std::wstring, std::wstring> vars;

	vars[L"(DRIVE)"] = drive.toStdWString();
	vars[L"(DIRECTORY)"] = directory.toStdWString();
	vars[L"(BOOT_FILE)"] = L"";

	// ps1 and ps2
	if (fileExists(directory + "/SYSTEM.CNF"))
	{
        WMiIni sysCnf((directory + "/SYSTEM.CNF").toStdWString(), false);

		// ps1
		if (sysCnf.exists(L"", L"BOOT"))
		{
			vars[L"(BOOT_FILE)"] = sysCnf.getStr(L"", L"BOOT");

			mediaType = "PS1 game disc";
			emulatorType = "PS1";
		}
		// ps2
		if (sysCnf.exists(L"", L"BOOT2"))
		{
			vars[L"(BOOT_FILE)"] = sysCnf.getStr(L"", L"BOOT2");

			mediaType = "PS2 game disc";
			emulatorType = "PS2";
		}
	}
	// psp
	else if (directoryExists(directory + "/PSP_GAME"))
	{
		mediaType = "PSP game disc";
		emulatorType = "PSP";
    }

    return tryLaunchEmu(emulatorType, mediaType, vars);
}

bool EmuDiscer::insertedMediaRaw(QString drive, const char *header, size_t headerSize)
{
    QString mediaType;
    QString emulatorType;
    std::map<std::wstring, std::wstring> vars;

    vars[L"(DRIVE)"] = drive.toStdWString();
    vars[L"(BOOT_FILE)"] = L"";

    // Gamecube (the bytes between 1B and 1F seem to always be the 00 C2 33 9F 3D)
    if (
        headerSize >= 32 &&
        memcmp(
            header+0x1b,
            "\x00\xC2\x33\x9F\x3D",
            5
        ) == 0
    )
    {
        mediaType = "Gamecube game disc";
        emulatorType = "Gamecube";
    }
    // Wii (the bytes between 17 and 1F seem to always be the 00 5D 1C 9E A3 00 00 00 00)
    else if (
        headerSize >= 32 &&
        memcmp(
            header+0x17,
            "\x00\x5D\x1C\x9E\xA3\x00\x00\x00\x00",
            9
        ) == 0
    )
    {
        mediaType = "Wii game disc";
        emulatorType = "Wii";
    }

    return tryLaunchEmu(emulatorType, mediaType, vars);
}


void EmuDiscer::on_trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	/*case QSystemTrayIcon::Trigger:
		mSysTrayMenu->popup(QCursor::pos());
		break;*/
    case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
		show();
		activateWindow();
		break;
	case QSystemTrayIcon::MiddleClick:
		QApplication::quit();
		break;
    default:
        break;
	}
}


void EmuDiscer::on_notificationClicked()
{
	show();
	activateWindow();
}

void EmuDiscer::on_mediaDirChanged(const QString &path)
{
    QStringList newDirs;
    /*QDirIterator it(path, QDirIterator::);

    while (it.hasNext())
    {
        newDirs.push_back(it.next());
    }*/
    QDir dir(path);

    for (const QString &str : dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
    {
        newDirs.push_back(path + "/" + str);
    }

    for (const QString &str : newDirs)
    {
        if (!mMediaDirectories.contains(str))
        {
            QString drive = getDrive(str);
            insertedMediaDir(str, drive);
        }
    }

    mMediaDirectories = newDirs;
}

void EmuDiscer::on_partitionChanged(QString drive)
{
    //"/dev/sr0";
    char headerBuffer[128];
    size_t charsToRead = 128;

    size_t charsRead = getDriveFileHeader(drive, headerBuffer, charsToRead);
    insertedMediaRaw(drive, headerBuffer, charsRead);
}

void EmuDiscer::on_driveMounted(QString drive, QString mountDir)
{
    insertedMediaDir(mountDir, drive);
}

void EmuDiscer::on_autoRunCheckbox_stateChanged(int state)
{
	mConfig.set(L"System", L"AutoRun", state == Qt::CheckState::Checked);
	mConfig.sync();
	setAutoRun(state == Qt::CheckState::Checked);
}

void EmuDiscer::on_notificationsCheckbox_stateChanged(int state)
{
	mConfig.set(L"System", L"ShowNotifications", state == Qt::CheckState::Checked);
}

/*void EmuDiscer::on_emulatorList_currentTextChanged(const QString& currentText)
{
	if (ui.emulatorList->selectedItems().size())
	{
		mSelectedEmulator = currentText.toStdWString();
		//ui.emulatorGroupbox->setEnabled(true);
		ui.pathEdit->setText(QString::fromStdWString(mConfig.getStr(mSelectedEmulator, L"Path")));
		ui.optionsEdit->setText(QString::fromStdWString(mConfig.getStr(mSelectedEmulator, L"Options")));
	}
	else
	{
		//ui.emulatorGroupbox->setEnabled(false);
		ui.pathEdit->setText("");
		ui.optionsEdit->setText("");
	}
}*/

void EmuDiscer::on_emulatorsTabWidget_currentChanged(int index)
{
	mSelectedEmulator = ui.emulatorsTabWidget->tabBar()->tabText(index).toStdWString();

	ui.emulatorsTabWidget->widget(index)->setLayout(ui.emulatorSettingsLayout);

    ui.optionsEdit->setText(QString::fromStdWString(mConfig.getStr(mSelectedEmulator, L"Options")));// this goes before pathEdit
    ui.pathEdit->setText(QString::fromStdWString(mConfig.getStr(mSelectedEmulator, L"Path")));

    /*Add built-in options*/
    //emulatorChanged();
    emulatorChanged();
}

void EmuDiscer::emulatorChanged()
{
    for (int i=0; i<ui.launchOptionCheckboxesLayout->count(); i++)
    {
        auto li = ui.launchOptionCheckboxesLayout->itemAt(i);
        auto w = li->widget();
        if (w)
        {
            ui.launchOptionCheckboxesLayout->removeItem(li);
            delete w;
            i--;
        }
    }

    for (auto &appOptionsPair : OptionsPerProgram)
    {
        if (ui.pathEdit->text().toLower().contains(appOptionsPair.first))
        {
            for (auto &textOptionPair : appOptionsPair.second)
            {
                QCheckBox *c = new QCheckBox(textOptionPair.first, this);
                c->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                c->setProperty("option", textOptionPair.second);

                QString spacedStr = tr(" ") + ui.optionsEdit->text() + tr(" ");

                c->setChecked(spacedStr.contains(textOptionPair.second));
                connect(c, SIGNAL(stateChanged(int)), this, SLOT(optionCheckbox_triggered(int)));
                /*if (ui.optionsEdit->text().size() == 0)
                {
                    c->setChecked(!textOptionPair.first.contains("not recommended"));
                }*/

                ui.launchOptionCheckboxesLayout->addWidget(c);
            }
            break;
        }
    }
}

void EmuDiscer::on_pathEdit_textChanged()
{
	if (mSelectedEmulator != L"")
	{
		mConfig.setStr(mSelectedEmulator, L"Path", ui.pathEdit->text().toStdWString());
		mConfig.sync();
		checkIfPathOk(ui.pathEdit);
	}
    emulatorChanged();
}

void EmuDiscer::on_pathButton_clicked()
{
    QFileDialog dialog;
    QString filename = dialog.getOpenFileName(this, "Select executable", ui.pathEdit->text(), getExeFileFilter());

    if (filename.size())
    {
        ui.pathEdit->setText(QString("\"") + filename + "\"");
    }
}

void EmuDiscer::on_appsButton_clicked()
{
    AppDialog dialog;
    dialog.setWindowIcon(windowIcon());
    dialog.exec();

    if (dialog.appPath().size())
        ui.pathEdit->setText(dialog.appPath());
}

void EmuDiscer::on_macrosButton_clicked()
{
    QAction *action = mMacrosMenu->exec(cursor().pos());
    if (action)
    {
        ui.optionsEdit->insert(action->text());
    }
}

void EmuDiscer::on_optionsEdit_textChanged()
{
	if (mSelectedEmulator != L"")
	{
		mConfig.setStr(mSelectedEmulator, L"Options", ui.optionsEdit->text().toStdWString());
		mConfig.sync();
	}
}


void EmuDiscer::on_saveStateRequest(QSessionManager &manager)
{
    manager.setRestartHint(QSessionManager::RestartHint::RestartNever);
}

void EmuDiscer::optionCheckbox_triggered(int state)
{
    QCheckBox *tc = (QCheckBox*)sender();
    QString spacedStr = tr(" ") + ui.optionsEdit->text() + tr(" ");

    if (state)
    {
        // find the previous option
        QCheckBox *prev = nullptr;
        for (int i=0; i<ui.launchOptionCheckboxesLayout->count(); i++)
        {
            auto li = ui.launchOptionCheckboxesLayout->itemAt(i);
            auto w = li->widget();
            if (w)
            {
                QCheckBox *c = (QCheckBox*)w;
                if (c == tc)
                {
                    break;
                }
                else if (c->isChecked())
                {
                    prev = c;
                }
            }
        }

        // find the prev text position
        int pos = 0;
        if (prev)
        {
            pos = spacedStr.indexOf(prev->property("option").toString());
            if (pos == -1)
                pos = 0;
            else
                pos += prev->property("option").toString().size();
        }

        // insert text
        spacedStr.insert(pos, tc->property("option").toString());
        ui.optionsEdit->setText(spacedStr.trimmed());
    }
    else
    {
        spacedStr.remove(tc->property("option").toString());
        ui.optionsEdit->setText(spacedStr.trimmed());
    }
}



bool checkIfPathOk(QLineEdit* lne)
{
    QString txt = lne->text();
    QString programFile = lne->text();

    bool ok = fileExists(programFile);
	QPalette palette = lne->style()->standardPalette();

	if (!ok && lne->text() != "")// error color
	{
		palette.setColor(QPalette::Base, Qt::red);
		//palette.setColor(QPalette::Text, Qt::white);
	}

	lne->setPalette(palette);
	return ok;
}
