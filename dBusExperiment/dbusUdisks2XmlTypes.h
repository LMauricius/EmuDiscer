#ifndef DBUSUDISKS2XMLTYPES_H
#define DBUSUDISKS2XMLTYPES_H

#include <QString>
#include <QVariant>
#include <QMap>
#include <QList>
#include <QPair>

#ifdef __unix__
#include <QtDBus>
#endif

// The weird characters are allowed unicode letters

typedef QMap<QString, QVariant> aEsv3_t;// a{sv}
//struct CsaƐsv3Ↄ { QString str; aƐsv3 map; };// (sa{sv})
typedef QPair<QString, aEsv3_t> CsaEsv3J_t;// (sa{sv})
typedef QList<CsaEsv3J_t> aCsaEsv3J_t;// a(sa{sv})
typedef QList<QList<quint8>> aay_t;// aay
typedef QPair<bool, QString> CbsJ_t;// (bs)
typedef QPair<QPair<bool, quint64>, QString> CbtsJ_t;// (bts)

Q_DECLARE_METATYPE(aEsv3_t)
Q_DECLARE_METATYPE(CsaEsv3J_t)
Q_DECLARE_METATYPE(aCsaEsv3J_t)
Q_DECLARE_METATYPE(aay_t)
Q_DECLARE_METATYPE(CbsJ_t)
Q_DECLARE_METATYPE(CbtsJ_t)

inline void registerUDisks2DbusTypes()
{
    qRegisterMetaType<aEsv3_t>();
    qRegisterMetaType<CsaEsv3J_t>();
    qRegisterMetaType<aCsaEsv3J_t>();
    qRegisterMetaType<aay_t>();
    qRegisterMetaType<CbsJ_t>();
    qRegisterMetaType<CbtsJ_t>();

    qDBusRegisterMetaType<aEsv3_t>();
    qDBusRegisterMetaType<CsaEsv3J_t>();
    qDBusRegisterMetaType<aCsaEsv3J_t>();
    qDBusRegisterMetaType<aay_t>();
    qDBusRegisterMetaType<CbsJ_t>();
    qDBusRegisterMetaType<CbtsJ_t>();
}

#endif // DBUSUDISKS2XMLTYPES_H
