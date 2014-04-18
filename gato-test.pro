TEMPLATE = app
TARGET = gato-test
QT -= gui

CONFIG   += console
CONFIG   -= app_bundle

SOURCES += main.cpp \
    tester.cpp

LIBS += -L$$OUT_PWD -lgato
INCLUDEPATH += $$PWD/../libgato
DEPENDPATH += $$PWD/../libgato

HEADERS += \
    tester.h

contains(MEEGO_EDITION,harmattan) {
    target.path = /opt/gato-test/bin
    INSTALLS += target
}

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog
