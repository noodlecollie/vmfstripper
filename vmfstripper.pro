#-------------------------------------------------
#
# Project created by QtCreator 2015-02-26T16:33:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vmfstripper
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    keyvaluesnode.cpp \
    keyvaluesparser.cpp

HEADERS  += mainwindow.h \
    keyvaluesnode.h \
    keyvaluesparser.h

FORMS    += mainwindow.ui
