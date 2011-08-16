#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T06:21:35
#
#-------------------------------------------------

#-------------------------------------------------
#
# TODO list
#
# Think about a better way to specify the project.
# Today it just builds whatever the main file specifies.
#
# 1 add file change save for exit dialog
# 2 add font resize
# 3 documentation: add xBasic help
#
# - documentation: describe purpose of classes methods
# - clean up mainwindow class
# - user options should have way to change syntax highlighting?
# - add compiler to gui?
# - set default compiler properties for windows install?
#
#-------------------------------------------------

QT += core gui

TARGET = xbasic-qt
TEMPLATE = app

DEFINES += QEXTSERIALPORT_LIB

INCLUDEPATH += \
    ../src/common \
    ../src/loader \
    ../src/runtime

SOURCES += main.cpp\
    mainwindow.cpp \
    highlighter.cpp \
    treemodel.cpp \
    treeitem.cpp \
    qextserialport.cpp \
    PortListener.cpp \
    terminal.cpp \
    properties.cpp \
    xbasicconfig.cpp \
    xbasicboard.cpp \
    console.cpp \
    hardware.cpp

HEADERS  += mainwindow.h \
    highlighter.h \
    treemodel.h \
    treeitem.h \
    qextserialport.h \
    qextserialenumerator.h \
    qextserialport_global.h \
    PortListener.h \
    terminal.h \
    properties.h \
    xbasicconfig.h \
    xbasicboard.h \
    console.h \
    hardware.h

OTHER_FILES +=

RESOURCES += \
    resources.qrc

unix:SOURCES       += posix_qextserialport.cpp
unix:!macx:SOURCES += qextserialenumerator_unix.cpp
macx {
  SOURCES          += qextserialenumerator_osx.cpp
  LIBS             += -framework IOKit -framework CoreFoundation
}
win32 {
  SOURCES          += win_qextserialport.cpp qextserialenumerator_win.cpp
  DEFINES          += WINVER=0x0501 # needed for mingw to pull in appropriate dbt business...probably a better way to do this
  LIBS             += -lsetupapi
}

FORMS += \
    properties.ui \
    hardware.ui
