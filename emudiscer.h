#ifndef EMUDISCER_H
#define EMUDISCER_H

#include <QtWidgets/QDialog>
#include <qsystemtrayicon.h>
#include <qmenu.h>
#include <QFileSystemWatcher>
#include <QAbstractNativeEventFilter>
#include <QTemporaryDir>
#include <QSessionManager>

#include "ui_emudiscer.h"
#include "MiIni.h"

class EmuDiscer : public QDialog/*, public QAbstractNativeEventFilter*/
{
	Q_OBJECT

public:
	EmuDiscer(QWidget *parent = 0);
	~EmuDiscer();

private:
	Ui::EmuDiscerClass ui;
    WMiIni mConfig;
	QMenu* mSysTrayMenu;
    QMenu* mMacrosMenu;
	QSystemTrayIcon* mSysTrayIcon;
    QTemporaryDir mTempDir;

	std::wstring mSelectedEmulator;

    QStringList mMediaDirectories;
    QStringList mPartitions;

	void closeEvent(QCloseEvent *event);
    bool nativeEvent(const QByteArray & eventType, void * message, long *result);

    /**
     * @param directory The directory of the disc files
     * @param drive The drive letter (Win) or device file (unix)
     * @param header The first bytes of the inserted drive
     * @param headerSize The number of bytes read and stored in header
    */
    bool insertedMediaDir(QString directory, QString drive);
    bool insertedMediaRaw(QString drive, const char *header, size_t headerSize);
    bool tryLaunchEmu(QString emulatorType, QString mediaType, const std::map<std::wstring, std::wstring>& vars);
    void emulatorChanged();

private slots:
	void on_trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void on_notificationClicked();
    void on_mediaDirChanged(const QString &path);
    void on_partitionChanged(QString drive);
    void on_driveMounted(QString drive, QString mountDir);

	void on_autoRunCheckbox_stateChanged(int state);
	void on_notificationsCheckbox_stateChanged(int state); 

	//void on_emulatorList_currentTextChanged(const QString& currentText);
	void on_emulatorsTabWidget_currentChanged(int index);

	void on_pathEdit_textChanged();
	void on_pathButton_clicked();
    void on_appsButton_clicked();
    void on_macrosButton_clicked();
    void on_optionsEdit_textChanged();

    void on_saveStateRequest(QSessionManager &manager);

    void optionCheckbox_triggered(int state);
};

bool checkIfPathOk(QLineEdit* lne);

#endif // EMUDISCER_H
