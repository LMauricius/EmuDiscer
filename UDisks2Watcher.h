#ifndef UDISKS2WATCHER_H
#define UDISKS2WATCHER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <map>

class UDisks2PropertyChangeWatcher;

class UDisks2Watcher : public QObject
{
    Q_OBJECT
public:
    explicit UDisks2Watcher(QObject *parent = nullptr);
    ~UDisks2Watcher();

signals:
    void partitionChanged(QString devName);

protected:
    QList<QString> mBlockdevInterfaces, mDriveInterfaces;
    std::map<QString, UDisks2PropertyChangeWatcher*> mPathWatcherMap;

    void updateInterfaceList();

protected slots:
    void on_blockDevicePropertiesChanged(QString objPath, QVariantMap changedProps);
    void on_interfaceAdded(QString objPath);
    void on_interfaceRemoved(QString objPath);
};


class UDisks2PropertyChangeWatcher : public QObject
{
    Q_OBJECT
public:
    explicit UDisks2PropertyChangeWatcher(QString objPath, QObject *parent = nullptr);
    ~UDisks2PropertyChangeWatcher();

signals:
    void objectPropertiesChanged(QString devName, QVariantMap changedProps);

protected:
    QString mObjPath;

protected slots:
    void on_propertiesChanged(QString interface, QVariantMap changedProps);
};

#endif // UDISKS2WATCHER_H
