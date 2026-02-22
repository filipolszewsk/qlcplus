QT += core network

TARGET = qlc-encrypt-content
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += ../../engine/src

SOURCES += main.cpp \
           ../../engine/src/qlccrypto.cpp \
           ../../engine/src/crypto/aes.c

HEADERS += ../../engine/src/qlccrypto.h
