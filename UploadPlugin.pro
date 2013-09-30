QT       += network
TEMPLATE = lib
TARGET = UploadPlugin
CONFIG += plugin
CONFIG -= app_bundle
version = 1.0.0

ROOT = ../../../
BIN = ../../bin

INCLUDEPATH = $$ROOT/interfaces
DESTDIR = $$BIN
MOC_DIR = moc
OBJECTS_DIR = obj
SOURCES += uploadplugin.cpp \
    json.cpp
HEADERS += uploadplugin.h \
    $$ROOT/interfaces/uploader.h \
    json.h
