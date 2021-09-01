#ifdef __unix__

#include "UDisks2Watcher.h"
#include <QtDBus>

namespace {
    QString ServiceName = "org.freedesktop.UDisks2";
    QString PathPrefix = "/org/freedesktop/UDisks2";
}



UDisks2Watcher::UDisks2Watcher(QObject *parent) : QObject(parent)
{
    updateInterfaceList();


    QDBusConnection::systemBus().connect(
        ServiceName, PathPrefix,
        "org.freedesktop.DBus.ObjectManager", "InterfacesAdded",
        this, SLOT(interfaceAdded(QString))
    );
    QDBusConnection::systemBus().connect(
        ServiceName, PathPrefix,
        "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved",
        this, SLOT(interfaceRemoved(QString))
    );


}

UDisks2Watcher::~UDisks2Watcher()
{
    for (auto pathWatcherPair : mPathWatcherMap)
    {
        disconnect(
            pathWatcherPair.second, SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
            this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
        );
        delete pathWatcherPair.second;
    }
}

void UDisks2Watcher::updateInterfaceList()
{
    // Clear
    mBlockdevInterfaces.clear();
    mDriveInterfaces.clear();
    for (auto pathWatcherPair : mPathWatcherMap)
    {
        disconnect(
            pathWatcherPair.second, SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
            this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
        );
        delete pathWatcherPair.second;
    }
    mPathWatcherMap.clear();

    // Call object getter
    QDBusMessage reply = QDBusConnection::systemBus().call(
        QDBusMessage::createMethodCall(
            ServiceName, PathPrefix,
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects"
        )
    );

    // List objects
    if (reply.type() != QDBusMessage::ErrorMessage && reply.arguments().size() >= 1)
    {
        qDebug() << "Listed UDisks2 objects";
        QVariantMap pathInterfacesMap = qdbus_cast<QVariantMap>(reply.arguments().at(0));

        for (auto pathInterIt = pathInterfacesMap.keyValueBegin(); pathInterIt != pathInterfacesMap.keyValueEnd(); pathInterIt++)
        {
            on_interfaceAdded(pathInterIt->first);
        }
    }
    else {
        qDebug() << "Error listing UDisks2 objects";
    }
}

void UDisks2Watcher::on_interfaceAdded(QString objPath)
{
    if (objPath.startsWith(PathPrefix+"/block_devices")) {
        mBlockdevInterfaces.append(objPath);

        if (mPathWatcherMap.find(objPath) == mPathWatcherMap.end())
        {
            auto newWatcher = new UDisks2PropertyChangeWatcher(objPath, this);
            mPathWatcherMap[objPath] = newWatcher;
            connect(
                newWatcher, SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
                this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
            );
        }

        emit deviceChanged(objPath.split('/').last());
    }
    else if (objPath.startsWith(PathPrefix+"/drives")) {
        mDriveInterfaces.append(objPath);
    }
}

void UDisks2Watcher::on_interfaceRemoved(QString objPath)
{
    if (objPath.startsWith(PathPrefix+"/block_devices")) {
        mBlockdevInterfaces.removeAll(objPath);
    }
    else if (objPath.startsWith(PathPrefix+"/drives")) {
        mDriveInterfaces.removeAll(objPath);
    }

    auto it = mPathWatcherMap.find(objPath);
    if (it != mPathWatcherMap.end())
    {
        disconnect(
            it->second, SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
            this, SLOT(objectPropertiesChanged(QString,QVariantMap))
        );

        delete it->second;
        mPathWatcherMap.erase(it);
    }
}

void UDisks2Watcher::on_blockDevicePropertiesChanged(QString objPath, QVariantMap changedProps)
{
    // sanity check, if drive is ok
    if (changedProps.contains("Size") && changedProps["Size"].toUInt() > 0)
    {
        emit partitionChanged(tr("/dev/") + objPath.split('/').last());
    }
}



UDisks2PropertyChangeWatcher::UDisks2PropertyChangeWatcher(QString objPath, QObject *parent):
    QObject(parent),
    mObjPath(objPath)
{
    QDBusConnection::systemBus().connect(
        ServiceName, mObjPath,
        "org.freedesktop.DBus.Properties", "PropertiesChanged",
        this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
    );
}

UDisks2PropertyChangeWatcher::~UDisks2PropertyChangeWatcher()
{
    QDBusConnection::systemBus().disconnect(
        ServiceName, mObjPath,
        "org.freedesktop.DBus.Properties", "PropertiesChanged",
        this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
    );
}

void UDisks2PropertyChangeWatcher::on_propertiesChanged(QString interface, QVariantMap changedProps)
{
    emit objectPropertiesChanged(mObjPath, changedProps);
}


#endif
