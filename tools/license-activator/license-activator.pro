QT += core gui widgets network

TARGET = qlc-license-activator
TEMPLATE = app

INCLUDEPATH += ../../engine/src

SOURCES += main.cpp \
           activatorwindow.cpp \
           ../../engine/src/qlccrypto.cpp \
           ../../engine/src/crypto/aes.c

HEADERS += activatorwindow.h \
           ../../engine/src/qlccrypto.h
