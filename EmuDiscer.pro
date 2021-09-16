QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32 {
    QT   += winextras
    #win32:LIBS += -luser32
}

unix {
    QT += dbus
    #DBUS_ADAPTORS += UDisks2_adaptor
    #DBUS_INTERFACES += UDisks2_interface
    #UDisks2_adaptor.files = org.freedesktop.UDisks2.xml
    #UDisks2_adaptor.header_flags = -idbusUdisks2XmlTypes.h
    #UDisks2_adaptor.source_flags = -idbusUdisks2XmlTypes.h
    #UDisks2_interface.files = org.freedesktop.UDisks2.xml
    #UDisks2_interface.header_flags = -idbusUdisks2XmlTypes.h
    #UDisks2_interface.source_flags = -idbusUdisks2XmlTypes.h

    CONFIG += console
}

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RC_ICONS = EmuDiscer.ico

SOURCES += \
    UDisks2Watcher.cpp \
    Utilities.cpp \
    appdialog.cpp \
    emudiscer.cpp \
    main.cpp

HEADERS += \
    EmulatorBuiltInOptions.h \
    MiIni.h \
    UDisks2Watcher.h \
    Utilities.h \
    appdialog.h \
    emudiscer.h

FORMS += \
    appdialog.ui \
    emudiscer.ui

TRANSLATIONS += \
    EmuDiscer_en_US.ts
CONFIG += lrelease
CONFIG += embed_translations

# Version
VERSION = 1.1
DEFINES += EMUDISCER_APP_VERSION=\\\"$$VERSION\\\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES +=
