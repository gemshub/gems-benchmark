TEMPLATE = app
LANGUAGE = C++
TARGET = difftest

CONFIG -= qt
CONFIG += warn_on
CONFIG += thread console
CONFIG += c++20
#CONFIG += sanitaze sanitaze_thread

#DEFINES += NO_NODEARRAYLEVEL
#DEFINES += USE_NLOHMANNJSON
#DEFINES += NO_THERMOFUN
#DEFINES += USE_THERMO_LOG
DEFINES += OVERFLOW_EXCEPT  #compile with nan inf exceptions
#DEFINES += SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#DEFINES += ADD_GEMS

SRC_CPP = $$PWD/src
SRC_H   = $$PWD/include
DEPENDPATH += $$SRC_H
INCLUDEPATH += $$SRC_H

contains(DEFINES, ADD_GEMS) {
GEMS3K_CPP = ../../GEMS3K/GEMS3K
GEMS3K_H   = $$GEMS3K_CPP
DEPENDPATH += $$GEMS3K_H
INCLUDEPATH += $$GEMS3K_H
include($$GEMS3K_CPP/gems3k.pri)
} else {
LIBS += -lGEMS3K
}

!contains(DEFINES, NO_THERMOFUN) {
LIBS += -lThermoFun -lChemicalFun
message("NO_THERMOFUN is not defined.")
} ## end NO_THERMOFUN

OBJECTS_DIR = obj

include($$SRC_CPP/difftest.pri)

SOURCES += \
        main.cpp \
        #tools/recalc_all.cpp \
        #tools/thread_test.cpp
        #tools/compare_dirs.cpp

DISTFILES += \
    doc/template_diff.json \
    doc/template_diff.md \
    doc/to_do.md
