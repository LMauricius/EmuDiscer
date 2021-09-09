#include "Utilities.h"

#include <qsettings.h>
#include <qcoreapplication.h>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QDebug>

#include <fstream>

#ifdef _WIN32
    #include "windows.h"
    #include <QtWin>
#elif defined __unix__
    #include <mntent.h>
#endif


QString getExeFileFilter()
{
#ifdef _WIN32
    return "Executable(*.exe;*.bat);;All Files(*.*)";
#elif defined __unix__
    return "All Files(*.*);;Shell script(*.sh)";
#endif
}

QString getDesktopFileName()
{
    return "LMauricius.EmuDiscer.desktop";
}

QString getHomeDirectory()
{
    return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
}

QString getConfigDirectory()
{
    return QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation).first();
}

QString getUsername()
{
    return getHomeDirectory().split(QDir::separator()).last();
}

QStringList getAppDirectories()
{
    QStringList lst = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
#ifdef _WIN32
    char *progDataC = getenv("ProgramData");
    if (progDataC)
    {
        QString progData = QString::fromStdString(progDataC);
        lst.push_back(progData + "\\Microsoft\\Windows\\Start Menu\\Programs");
    }
#endif
    return lst;
}

QStringList getRemovableDriveDirectories()
{
#ifdef _WIN32
    return {};
#elif defined __unix__
    return {
        QString("/run/media/") + getUsername(),
        QString("/media/") + getUsername()
    };
#endif
}

AppDef getApps()
{
    AppDef root;

#ifdef _WIN32

    for (const QString &path : getAppDirectories())
    {
        QDir dir(path);

        QDirIterator it(path, QDir::Filter::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString fstr = it.next();
            QFileInfo info(fstr);
            if (info.isSymLink())
            {
                QString name = info.baseName()/*.remove(".lnk")*/;
                QString exec = QString("\"")+info.symLinkTarget()+"\"";
                QIcon icon;

                // extract icon
                {
                    QString natFStr = QDir::toNativeSeparators(info.symLinkTarget());
                    wchar_t *cFilename = new wchar_t[natFStr.length()+1];
                    natFStr.toWCharArray(cFilename);
                    cFilename[natFStr.length()] = 0;

                    const UINT numIcons = ExtractIconExW(cFilename, -1, nullptr, nullptr, 0);
                    if (numIcons > 0)
                    {
                        QScopedArrayPointer<HICON> winIcons(new HICON[numIcons]);
                        const UINT numExtrIcons = ExtractIconExW(cFilename, 0, winIcons.data(), nullptr, 1);

                        if (numExtrIcons > 0)
                        {
                            icon = QtWin::fromHICON(winIcons[0]);
                        }
                    }

                    delete[] cFilename;
                }

                AppDef *parent = &root;
                QString relPath = info.absoluteFilePath().mid(path.size()+1);
                QStringList parentNameList = relPath.split(QRegExp("[/\\\\]"));
                parentNameList.pop_back();
                for (auto& cat : parentNameList)
                {
                    parent = &(parent->subApps[cat]);
                }
                parent->subApps[name] = AppDef(exec, icon);
            }
        }
    }
#elif defined __unix__
    std::list<WMiIni> settingsess;
    std::map<QString, int> refs;

    for (const QString &path : getAppDirectories())
    {
        QDir dir(path);

        for (const QString &fstr : dir.entryList(QDir::Filter::Files|QDir::Filter::NoDotAndDotDot))
        {
            QFileInfo info(path + "/" + fstr);

            if (info.suffix() == "desktop")
            {
                // Qt has better unicode reading support than MiIni, so we use read() with loaded text
                WMiIni appInfo(L"", false);

                QFile file(info.filePath());
                file.open(QIODevice::ReadOnly | QIODevice::Text);

                QTextStream qts(&file);
                QString text;
                while (!qts.atEnd())
                    text += qts.readLine()+"\n";
                std::wstringstream ss(text.toStdWString());
                appInfo.read(ss);

                file.close();

                if (appInfo.getStr(L"Desktop Entry", L"Type") == L"Application")
                {
                    settingsess.push_back(appInfo);
                    QStringList cats = QString::fromStdWString(appInfo.getStr(L"Desktop Entry", L"Categories")).split(';');
                    for (const QString &cat : cats)
                    {
                        if (cat.size())
                        {
                            if (refs.find(cat) == refs.end())
                            {
                                refs[cat] = 1;
                            }
                            else
                            {
                                refs[cat]++;
                            }
                        }
                    }
                }
            }
        }
    }

    for (WMiIni &appInfo : settingsess)
    {
        QStringList cats = QString::fromStdWString(appInfo.getStr(L"Desktop Entry", L"Categories")).split(';');

        if (cats.size())
        {
            int maxRefs = 0;
            QString cat = "";
            for (const QString &c : cats)
            {
                if (c.size())
                {
                    int r = refs[c];
                    if (r > maxRefs)
                    {
                        maxRefs = r;
                        cat = c;
                    }
                }
            }
            if (cat.size())
            {
                QString name = QString::fromStdWString(appInfo.getStr(L"Desktop Entry", L"Name"));
                QString exec = QString::fromStdWString(appInfo.getStr(L"Desktop Entry", L"Exec"));
                QString iconFile = QString::fromStdWString(appInfo.getStr(L"Desktop Entry", L"Icon"));
                root.subApps[cat].subApps[name] = AppDef(exec, QIcon::fromTheme(iconFile));
            }
        }
    }
#endif

    return root;
}

QString getDrive(const QString& mountPoint)
{
#ifdef _WIN32
    return QString(mountPoint[0]);
#elif defined __unix__
    FILE *file = setmntent("/proc/mounts", "r");
    mntent *entry = nullptr;

    if (file)
    {
        while ((entry = getmntent(file)))
        {
            if (entry->mnt_dir && mountPoint == entry->mnt_dir)
            {
                endmntent(file);
                return QString::fromStdString(entry->mnt_fsname);
            }
        }
    }

    endmntent(file);
    return "";
#endif
}

// accepts drive letter (windows) or drive file (unix); returns chars read
size_t getDriveFileHeader(QString drive, char *headerBuffer, size_t charsToRead)
{
#ifdef _WIN32
    std::string driveFilename = std::string("\\\\.\\")+drive.toStdString()+":";
#elif defined __unix__
    std::string driveFilename = drive.toStdString();
#endif

    size_t charsRead = 0;
    memset(headerBuffer, 0, charsToRead);

    std::ifstream driveFile(driveFilename, std::ios::binary);
    if (driveFile.good()) {
        driveFile.read(headerBuffer, charsToRead);
        if (driveFile)
        {
            charsRead = charsToRead;
        }
        else
        {
            charsRead = driveFile.gcount();
        }
        driveFile.close();
    }
    else {
        std::string reasons = strerror(errno);
        qCritical() << "Couldn't open file: " << QString::fromStdString(reasons) << "\n";
    }

    return charsRead;
}

void setAutoRun(bool autorun)
{
#ifdef _WIN32
	QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

	if (autorun)
	{
		QString value = QCoreApplication::applicationFilePath();

		value.replace("/", "\\");
		value = QString("\"") + value + "\"";

		//write value to the register
		settings.setValue("EmuDiscer", value);
	}
	else
	{
		settings.remove("EmuDiscer");
    }
#elif defined __unix__
    QString target = getHomeDirectory() + "/.config/autostart/LMauricius.EmuDiscer.desktop";

    if (autorun)
    {
        QString progPath = QString("\"%1\"").arg(qApp->arguments().first());

        //QFile::copy(":/desktop/LMauricius.EmuDiscer.desktop", target);
        WMiIni desktop(target.toStdWString(), false);
        desktop.setStr(L"Desktop Entry", L"Exec", progPath.toStdWString());
        desktop.setStr(L"Desktop Entry", L"Name", L"EmuDiscer");
        desktop.setStr(L"Desktop Entry", L"Icon", L"EmuDiscer");
        desktop.setStr(L"Desktop Entry", L"Type", L"Application");
        desktop.setStr(L"Desktop Entry", L"Terminal", L"false");
        desktop.setStr(L"Desktop Entry", L"Categories", L"Game;");
        desktop.setStr(L"Desktop Entry", L"Comment", L"A program for running emulators when a disc is inserted.");
        desktop.sync();
        QFile::setPermissions(target, QFile::permissions(target) | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther);
    }
    else
    {
        QFile file(target);
        file.remove();
    }
#endif
}


bool fileExists(const QString& name)
{
    if (name.size() > 0)
    {
        QString command;
        if (name.size() > 1 && name[0] == '\"')
            command = name.mid(1, name.indexOf('\"', 1)-1);
        else if (name.size() > 1 && name[0] == '\'')
            command = name.mid(1, name.indexOf('\'', 1)-1);
        else
            command = name.left(name.indexOf(' ', 1));

        QFileInfo info(name), info2(command);

        if ((info.exists() && info.isFile()) || (info2.exists() && info2.isFile()))
            return true;

#ifdef __unix__

        if (
            !command.contains(' ') &&
            system(
                (QString("which ") + command + " > /dev/null 2>&1").toStdString().c_str()
            ) == 0
        )
            return true;
#endif
    }

    return false;
}

bool directoryExists(const QString& name)
{
	QFileInfo info(name);
	return info.exists() && info.isDir();
}

void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void startProgram(const std::wstring& path, const std::wstring& options)
{
/*#ifdef WIN32
    std::wstring dir = path.substr(0, path.find_last_of(L"\\/"));

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcessW(path.c_str(), const_cast<wchar_t*>(options.c_str()),
        NULL, NULL, false, 0, NULL, dir.c_str(), &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    QProcess p(nullptr);
    p.startDetached(
        QString::fromStdWString(path) + QString::fromStdWString(options)
    );
#endif*/
    QProcess p(nullptr);
    p.startDetached(
        QString::fromStdWString(path) + " " + QString::fromStdWString(options)
    );
}
