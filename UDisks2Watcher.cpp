#include "UDisks2Watcher.h"
#include <stdexcept>

#ifdef __unix__
#include <QtDBus>
#endif

namespace {

#ifdef __unix__
    const QString ServiceName = "org.freedesktop.UDisks2";
    const QString PathPrefix = "/org/freedesktop/UDisks2";
    const QString ObjectManagerInterface = "org.freedesktop.DBus.ObjectManager";
    const QString PropertiesInterface = "org.freedesktop.DBus.Properties";

    QString blockDevObjPath2Drive(const QString& objPath)
    {
        return QString("/dev/") + objPath.split('/').last();
    }

    QStringList aay2StringList(const QVariant& aayContainingVariant)
    {
        QStringList ret;

        // The argument type is 'aay' which seems too complex for Qt to proccess, so we *simply* extract it
        // fml

        QDBusArgument argFirst;

        if (aayContainingVariant.canConvert<QDBusArgument>()) {
          argFirst = aayContainingVariant.value<QDBusArgument>();
        } else if (aayContainingVariant.canConvert<QDBusVariant>()) {
          QDBusVariant dbvFirst = aayContainingVariant.value<QDBusVariant>();
          QVariant vFirst = dbvFirst.variant();
          argFirst = vFirst.value<QDBusArgument>();
        } else {
          throw std::runtime_error(
              std::string("Can't retrieve QDBusArgument: variant is ") +
              aayContainingVariant.typeName());
        }

        argFirst.beginArray();

        while ( !argFirst.atEnd() ) {
            QByteArray content;
            argFirst >> content;
            if (content.endsWith('\0'))
              content.removeLast();
            ret.append(QString::fromUtf8(content));
        }

        argFirst.endArray();

        return ret;
    }

#endif
}



UDisks2Watcher::UDisks2Watcher(QObject *parent) : QObject(parent)
{

#ifdef __unix__

    updateInterfaceList();

    QDBusConnection::systemBus().connect(
        ServiceName, PathPrefix,
        "org.freedesktop.DBus.ObjectManager", "InterfacesAdded",
        this, SLOT(on_interfaceAdded(QString))
    );
    QDBusConnection::systemBus().connect(
        ServiceName, PathPrefix,
        "org.freedesktop.DBus.ObjectManager", "InterfacesRemoved",
        this, SLOT(on_interfaceRemoved(QString))
    );

#endif


}

UDisks2Watcher::~UDisks2Watcher()
{
}

#ifdef __unix__

void UDisks2Watcher::updateInterfaceList()
{
    // Clear
    mBlockdevObjects.clear();
    mPathWatcherMap.clear();

    // Call object getter
    QDBusMessage reply = QDBusConnection::systemBus().call(QDBusMessage::createMethodCall(
        ServiceName, PathPrefix,
        ObjectManagerInterface, "GetManagedObjects"
    ));

    // Remember objects
    if (reply.type() != QDBusMessage::ErrorMessage && reply.arguments().size() >= 1)
    {
        for (auto& pathInterPair : qdbus_cast<QVariantMap>(reply.arguments().at(0)).toStdMap())
        {
            on_interfaceAdded(pathInterPair.first);
        }
    }
    else {
        qDebug() << "Error listing UDisks2 objects";
    }
}

void UDisks2Watcher::on_interfaceAdded(QString objPath)
{
    // If this is a block device (partition)
    if (objPath.startsWith(PathPrefix+"/block_devices"))
    {
        mBlockdevObjects.removeAll(objPath);
        mDeviceMounts[objPath].clear();

        mBlockdevObjects.append(objPath);

        // create watcher & connect signal
        if (mPathWatcherMap.find(objPath) == mPathWatcherMap.end())
        {
            mPathWatcherMap[objPath] = std::make_unique<UDisks2PropertyChangeWatcher>(objPath);
            connect(
                mPathWatcherMap[objPath].get(), SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
                this, SLOT(on_blockDevicePropertiesChanged(QString,QVariantMap))
            );
        }

        // check mount points
        QDBusMessage reply = QDBusConnection::systemBus().call(QDBusMessage::createMethodCall(
            ServiceName, objPath,
            PropertiesInterface, "Get"
        ) << QString("org.freedesktop.UDisks2.Filesystem") << QString("MountPoints"));

        if (reply.type() != QDBusMessage::ErrorMessage)
        {
            if (reply.arguments().size() >= 1)
            {
                for (auto& mnt : aay2StringList(reply.arguments().at(0)))
                {
                    mDeviceMounts[objPath].push_back(mnt);
                }
            }
        }

        // partition signal
        emit partitionChanged(blockDevObjPath2Drive(objPath));
    }
}

void UDisks2Watcher::on_interfaceRemoved(QString objPath)
{
    if (objPath.startsWith(PathPrefix+"/block_devices"))
    {
        mBlockdevObjects.removeAll(objPath);

        auto it = mPathWatcherMap.find(objPath);
        if (it != mPathWatcherMap.end())
        {
            disconnect(
                it->second.get(), SIGNAL(objectPropertiesChanged(QString,QVariantMap)),
                this, SLOT(objectPropertiesChanged(QString,QVariantMap))
            );

            mPathWatcherMap.erase(it);
        }
    }
}

void UDisks2Watcher::on_blockDevicePropertiesChanged(QString objPath, QVariantMap changedProps)
{
    // sanity check, if drive is ok
    // size should always change to 0 when ejected, and change to non-0 when inserted
    if (changedProps.contains("Size") && changedProps["Size"].toUInt() > 0)
    {
        emit partitionChanged(blockDevObjPath2Drive(objPath));
    }

    if (changedProps.contains("MountPoints"))
    {
        // check mount points
        QDBusMessage reply = QDBusConnection::systemBus().call(QDBusMessage::createMethodCall(
            ServiceName, objPath,
            PropertiesInterface, "Get"
        ) << QString("org.freedesktop.UDisks2.Filesystem") << QString("MountPoints"));

        if (reply.type() != QDBusMessage::ErrorMessage)
        {
            if (reply.arguments().size() >= 1)
            {
                QStringList mnts = aay2StringList(changedProps["MountPoints"]);

                for (const QString& mnt : mnts)
                {
                    if (!mDeviceMounts[objPath].contains(mnt))
                    {
                        emit driveMounted(blockDevObjPath2Drive(objPath), mnt);
                    }
                }

                mDeviceMounts[objPath] = mnts;
            }
        }

    }
}



UDisks2PropertyChangeWatcher::UDisks2PropertyChangeWatcher(QString objPath, QObject *parent):
    QObject(parent),
    mObjPath(objPath)
{
    QDBusConnection::systemBus().connect(
        ServiceName, mObjPath,
        "org.freedesktop.DBus.Properties", "PropertiesChanged",
        this, SLOT(on_propertiesChanged(QString,QVariantMap))
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
