#-------------------------------------------------
#
# Project created by QtCreator 2011-07-29T19:32:31
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = xbcom-qt
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += Q_OS_WIN32

TEMPLATE = app

INCLUDEPATH += \
    ../src/common \
    ../src/loader \
    ../src/runtime

SOURCES += \
    ../src/common/db_system.c \
    ../src/common/db_platform.c \
    ../src/common/db_config.c \
    ../src/compiler/xbcom.c \
    ../src/compiler/db_wrimage.c \
    ../src/compiler/db_types.c \
    ../src/compiler/db_symbols.c \
    ../src/compiler/db_statement.c \
    ../src/compiler/db_scan.c \
    ../src/compiler/db_generate.c \
    ../src/compiler/db_expr.c \
    ../src/compiler/db_compiler.c \
    ../src/loader/PLoadLib.c \
    ../src/loader/db_packet.c \
    ../src/loader/db_loader.c \
    ../src/runtime/db_vmdebug.c \
    ../src/common/osint_qt.c \
    ../src/common/mem_malloc.c \
    ../src/compiler/xb_api.c \
    xbasic_vm.c \
    serial_helper.c \
    hub_loader.c \
    flash_loader.c \
    ../src/compiler/db_pasm.c

HEADERS += \
    ../src/common/osint.h \
    ../src/common/db_system.h \
    ../src/common/db_image.h \
    ../src/common/db_config.h \
    ../src/compiler/db_compiler.h \
    ../src/loader/PLoadLib.h \
    ../src/loader/db_packet.h \
    ../src/loader/db_loader.h \
    ../src/misc/db_spin.h \
    ../src/runtime/db_vmimage.h \
    ../src/runtime/db_vmdebug.h \
    ../src/runtime/db_vm.h \
    ../src/common/mem_malloc.h \
    ../src/compiler/xb_api.h
