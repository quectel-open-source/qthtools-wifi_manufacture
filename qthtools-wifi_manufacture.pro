QT       += core gui
QT       += network
QT       += serialport
QT       += concurrent
QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

include ($$PWD/third/QSimpleUpdater/QSimpleUpdater.pri)
include ($$PWD/third/LogModule/LogModule.pri)

INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/third

CONFIG += c++11
CONFIG += resources_big

QMAKE_CFLAGS_RELEASE += -g
QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_LFLAGS_RELEASE = -mthreads -W

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    app/burning.cpp \
    app/envsetting.cpp \
    kernel/filemanager.cpp \
    kernel/httpclient.cpp \
    kernel/jsonFile.cpp \
    kernel/mmessagebox.cpp \
    kernel/serial.cpp \
    main.cpp \
    mainwindow.cpp \
    third/qrwidget.cpp \
    third/switchbutton.cpp

HEADERS += \
    app/burning.h \
    app/envsetting.h \
    kernel/filemanager.h \
    kernel/httpclient.h \
    kernel/jsonFile.h \
    kernel/kernel_include.h \
    kernel/mmessagebox.h \
    kernel/serial.h \
    mainwindow.h \
    third/qrwidget.h \
    third/switchbutton.h

FORMS += \
    app/envsetting.ui \
    kernel/mmessagebox.ui \
    mainwindow.ui \
    third/qrwidget.ui

TRANSLATIONS += \
    chinese.ts english.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resoureces.qrc

RC_FILE += icon/quectel.rc

DISTFILES += \
    icon/quectel.ico \
    icon/quectel.rc \
