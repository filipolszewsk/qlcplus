QT += core network
CONFIG += console
CONFIG -= app_bundle
TARGET = test-crypto
TEMPLATE = app

INCLUDEPATH += ../../engine/src

SOURCES += main.cpp \
           ../../engine/src/qlccrypto.cpp \
           ../../engine/src/crypto/aes.c

HEADERS += ../../engine/src/qlccrypto.h
