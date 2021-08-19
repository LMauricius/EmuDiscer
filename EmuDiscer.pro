QT       += core gui
win32 {
    QT   += winextras
}◘

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Utilities.cpp \
    appdialog.cpp \
    emudiscer.cpp \
    main.cpp

HEADERS += \
    EmulatorBuiltInOptions.h \
    MiIni.h \
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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
