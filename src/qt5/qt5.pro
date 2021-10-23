TEMPLATE = app
CONFIG += qt 
QT += core gui svg
TARGET = resetmsmice-gui
INCLUDEPATH += ../../include/qt5
SOURCES += resetWindow.cc eventLoopProcess.cc execSave.cc main.cc terminalView.cc
HEADERS += ../../include/qt5/eventLoopProcess.h ../../include/qt5/execSave.h ../../include/qt5/resetWindow.h ../../include/qt5/terminalView.h 
ICON = ../../data/resetmsmice.icns
RC_ICONS = ../../data/resetmsmice.icns
QMAKE_INFO_PLIST = ../../data/Info.plist
RESOURCE_FILES_A.files = $$ICON
RESOURCE_FILES_A.path = Contents/Resources
RESOURCE_FILES_B.files = ../../resetmsmice.app/Contents/MacOS/resetmsmice
RESOURCE_FILES_B.path = Contents/Resources
RESOURCE_FILES_C.files = ../../macos/resetmsmice-enable-boot
RESOURCE_FILES_C.path = Contents/Resources
QMAKE_BUNDLE_DATA += RESOURCE_FILES_A RESOURCE_FILES_B RESOURCE_FILES_C
DESTDIR = ../..
QMAKE_POST_LINK = macdeployqt ../../$$TARGET\.app -qmldir=$$PWD
