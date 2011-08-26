#-------------------------------------------------
#
# Project created by QtCreator 2011-07-31T06:21:35
#
#-------------------------------------------------

#-------------------------------------------------
#
# TODO list
#
# Test using array.bas ... David says some output is missing.
#
# 2 make xbcom-qt build all from scratch
# 3 documentation: add xBasic help
#
# - add font resize
# - add comment editor to hardware dialog
# - add tools for spin compiler and bin2 blah
# - documentation: describe purpose of classes methods
# - clean up mainwindow class
# - user options should have way to change syntax highlighting?
# - set default compiler properties for windows install?
#   make the compiler and include path properties default settings.
# - add Ctrl-LeftClick to zoom on a function or constant def
#
# Think about a better way to specify the project. - done.
# add file change save for exit dialog - done
#-------------------------------------------------

QT += core gui

TARGET = xbasic-qt
TEMPLATE = app

DEFINES += QEXTSERIALPORT_LIB
DEFINES += IDEVERSION="0"
DEFINES += MINVERSION=1
DEFINES += FIXVERSION=7

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
    hardware.ui
