#-------------------------------------------------
#
# Project created by QtCreator 2013-08-17T23:09:57
#
#-------------------------------------------------

QT       += network
TEMPLATE = lib
TARGET = UploadPlugin
CONFIG += plugin
CONFIG -= app_bundle
version = 1.0.0

INCLUDEPATH = ../DownloadHost/interfaces
DESTDIR = ../DownloadHost/plugins

HEADERS += \
    uploadplugin.h \
    ../DownloadHost/interfaces/uploader.h \
    json.h

SOURCES += \
    uploadplugin.cpp \
    json.cpp

