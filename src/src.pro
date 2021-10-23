TEMPLATE = app
QT -= core gui
CONFIG += thread
TARGET = resetmsmice
INCLUDEPATH += ../include
SOURCES = resetmsmice.c parse.c pthread-misc.c main.c
HEADERS = ../include/basictypes.h ../include/hid-userland.h ../include/pthread-misc.h ../include/resetmsmice.h
CONFIG += link_pkgconfig
PKGCONFIG += libusb-1.0
LIBS += -lIconv
DESTDIR = ..
