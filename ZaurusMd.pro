TEMPLATE = app
CONFIG += qt warn_on release
TARGET = zaurusmd
DESTDIR = DIST

HEADERS = \
    src/MainWindow.h \
    src/MdEdit.h \
    src/MdView.h \
    src/MdParser.h \
    src/TodoTxt.h \
    src/FileUtil.h

SOURCES = \
    src/main.cpp \
    src/MainWindow.cpp \
    src/MdView.cpp \
    src/MdParser.cpp \
    src/TodoTxt.cpp \
    src/FileUtil.cpp

INCLUDEPATH += $(QPEDIR)/include
LIBS += -lqpe
