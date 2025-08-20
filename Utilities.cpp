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
#include <map>

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

QString getProgramPath()
{
    auto progPath = QCoreApplication::applicationFilePath();
#ifdef _WIN32
    if (progPath.size() && progPath[0] != '"' && progPath[0] != '\'')
        return QString("\"%1\"").arg(QCoreApplication::applicationFilePath());
    else
        return QCoreApplication::applicationFilePath();
#elif defined __unix__
    auto appimagePath = qgetenv("APPIMAGE");
    if (appimagePath.size())
    {
        return appimagePath;
    }
    if (progPath.size() && progPath[0] != '"' && progPath[0] != '\'')
        return QString("\"%1\"").arg(QCoreApplication::applicationFilePath());
    else
        return QCoreApplication::applicationFilePath();
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
    return QDir::fromNativeSeparators(getHomeDirectory()).split('/').last();
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

                // extract icon
                QString natFStr = QDir::toNativeSeparators(info.symLinkTarget());

                AppDef *parent = &root;
                QString relPath = info.absoluteFilePath().mid(path.size()+1);
                QStringList parentNameList = relPath.split(QRegExp("[/\\\\]"));
                parentNameList.pop_back();
                for (auto& cat : parentNameList)
                {
                    parent = &(parent->subApps[cat]);
                }
                parent->subApps[name] = AppDef(
                            exec,
                            [natFStr](){
                                QIcon icon;

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
                                return icon;
                            }
                );
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
                root.subApps[cat].subApps[name] = AppDef(
                            exec,
                            [iconFile](){
                                return QIcon::fromTheme(iconFile);
                            }
                );
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

bool isAutoRun()
{
    QString progPath = getProgramPath();

#ifdef _WIN32
    QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    progPath.replace("/", "\\");
    progPath = QString("\"") + progPath + "\"";

    return settings.contains("EmuDiscer") && settings.value("EmuDiscer") == progPath;
#elif defined __unix__
    QString target = getHomeDirectory() + "/.config/autostart/LMauricius.EmuDiscer.desktop";

    if (!QFile::exists(target))
        return false;
    else
    {
        WMiIni desktop(target.toStdWString(), false);
        return desktop.getStr(L"Desktop Entry", L"Exec", L"") == progPath.toStdWString();
    }
#endif
}

void setAutoRun(bool autorun)
{
    QString progPath = getProgramPath();

#ifdef _WIN32
	QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

	if (autorun)
    {
        progPath.replace("/", "\\");
        progPath = QString("\"") + progPath + "\"";

		//write value to the register
        settings.setValue("EmuDiscer", progPath);
	}
	else
	{
		settings.remove("EmuDiscer");
    }
#elif defined __unix__
    QString target = getHomeDirectory() + "/.config/autostart/LMauricius.EmuDiscer.desktop";

    if (autorun)
    {
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

void startProgram(const QString& path, const QString& options, const QString& workdir, const std::map<QString, QString>& envVars)
{
    /*QStringList firstOptions = QProcess::splitCommand(path);
    QStringList secondOptions = QProcess::splitCommand(options);
    QString prog = firstOptions.first();
    QStringList optionsList = QStringList(++firstOptions.begin(), firstOptions.end());
    optionsList.append(secondOptions);

    QProcess p(nullptr);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (const auto& varValPair : envVars)
    {
        env.insert(varValPair.first, varValPair.second);
    }
    p.setProgram(prog);
    p.setArguments(optionsList);
    if (workdir.size())
    {
        p.setWorkingDirectory(workdir);
    }
    p.setProcessEnvironment(env);*/

    QStringList splitComm = QProcess::splitCommand(path + " " + options);
    qDebug() << "Starting command `" << splitComm << "`\n";
    if (splitComm.size() > 0) {
      QProcess::startDetached(splitComm.first(), splitComm.sliced(1));
    }
}
