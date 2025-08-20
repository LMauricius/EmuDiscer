#ifndef UDISKS2WATCHER_H
#define UDISKS2WATCHER_H

#include <QMap>
#include <QObject>
#include <QVariant>
#include <map>
#include <memory>

class UDisks2PropertyChangeWatcher;

class UDisks2Watcher : public QObject
{
    Q_OBJECT
  public:
    explicit UDisks2Watcher(QObject *parent = nullptr);
    ~UDisks2Watcher();

  signals:
    void partitionChanged(QString driveName);
    void driveMounted(QString driveName, QString mountDir);

  protected:
#ifdef __unix__
    QList<QString> mBlockdevObjects;
    std::map<QString, std::unique_ptr<UDisks2PropertyChangeWatcher>> mPathWatcherMap;
    std::map<QString, QList<QString>> mDeviceMounts;

    void updateInterfaceList();
#endif

  protected slots:
#ifdef __unix__
    void on_blockDevicePropertiesChanged(QString objPath, QVariantMap changedProps);
    void on_interfaceAdded(QString objPath);
    void on_interfaceRemoved(QString objPath);
#endif
};

#ifdef __unix__
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
#endif

#endif // UDISKS2WATCHER_H
