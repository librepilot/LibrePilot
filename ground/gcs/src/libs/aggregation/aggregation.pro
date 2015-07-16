TEMPLATE = lib
TARGET = Aggregation

include(../../library.pri)

DEFINES += AGGREGATION_LIBRARY

HEADERS = aggregate.h \
    aggregation_global.h

SOURCES = aggregate.cpp

