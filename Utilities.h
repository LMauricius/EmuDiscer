#pragma once

#include <string>
#include <QString>
#include <QIcon>
#include <QTextStream>
#include <QFile>
#include <map>
#include "MiIni.h"

struct AppDef
{
    inline AppDef(QString path, std::function<QIcon()> iconLoader): path(path), iconLoader(iconLoader) {}
    AppDef() = default;
    AppDef(const AppDef&) = default;
    AppDef(AppDef&&) = default;

    AppDef& operator=(const AppDef&) = default;
    AppDef& operator=(AppDef&&) = default;

    QString path;
    std::function<QIcon()> iconLoader = {};

    std::map<QString, AppDef> subApps;
};

QString getExeFileFilter();
QString getProgramPath();
QString getDesktopFileName();
QString getHomeDirectory();
QString getConfigDirectory();
QString getUsername();
QStringList getAppDirectories();
QStringList getRemovableDriveDirectories();
AppDef getApps();

QString getDrive(const QString& mountPoint);
size_t getDriveFileHeader(QString drive, char *headerBuffer, size_t charsToRead);// accepts drive letter (windows) or drive file (unix); returns chars read
bool fileExists(const QString& name);
bool directoryExists(const QString& name);
void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
bool isAutoRun();
void setAutoRun(bool autorun);
void startProgram(const QString& path, const QString& options, const QString& workdir, const std::map<QString, QString>& envVars);
