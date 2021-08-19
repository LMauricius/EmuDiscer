#pragma once

#include <string>
#include <QString>
#include <QIcon>
#include <QTextStream>
#include <QFile>
#include <map>
#include "MiIni.h"

/*namespace QMiIniUtils
{
    class QMiIniReadFunc
    {
    public:
        void operator()(const QString& filename, std::function<void(QTextStream&)> streamReader) {
            QFile file = QFile(filename);

            if (file.isOpen()) {
                QTextStream qts(&file);
                streamReader(qts);
                file.close();
            }
        }
    };

    class QMiIniWriteFunc
    {
    public:
        void operator()(const QString& filename, std::function<void(QTextStream&)> streamWriter) {
            QFile file = QFile(filename);

            if (file.isOpen()) {
                QTextStream qts(&file);
                streamWriter(qts);
                file.close();
            }
            else {
                throw std::runtime_error(
                        ((std::stringstream&)(std::stringstream() <<
                            "Can't open ini file \"" << filename.toStdString() << "\"!")).str()
                );
            }
        }
    };
}

using QMiIni = MiIni<
    QString,
    QTextStream,
    QTextStream,
    QTextStream,
    QString,
    QMiIniUtils::QMiIniReadFunc,
    QMiIniUtils::QMiIniWriteFunc
>;*/

struct AppDef
{
    inline AppDef(QString path, QIcon icon): path(path), icon(icon) {}
    AppDef() = default;
    AppDef(const AppDef&) = default;
    AppDef(AppDef&&) = default;

    AppDef& operator=(const AppDef&) = default;
    AppDef& operator=(AppDef&&) = default;

    QString path;
    QIcon icon;

    std::map<QString, AppDef> subApps;
};

QString getExeFileFilter();
QString getDesktopFileName();
QString getHomeDirectory();
QString getConfigDirectory();
QString getUsername();
QStringList getAppDirectories();
QStringList getRemovableDriveDirectories();
AppDef getApps();

QString getDrive(const QString& mountPoint);
bool fileExists(const QString& name);
bool directoryExists(const QString& name);
void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
void setAutoRun(bool autorun);
void startProgram(const std::wstring& path, const std::wstring& options);
