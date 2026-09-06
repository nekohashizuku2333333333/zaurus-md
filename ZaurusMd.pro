TEMPLATE = app
CONFIG += qt warn_on release
TARGET = zaurusmd
DESTDIR = DIST

HEADERS = \
    src/MainWindow.h \
    src/MarkdownActions.h \
    src/MdEdit.h \
    src/MdView.h \
    src/TextPrompt.h \
    src/MdParser.h \
    src/TodoTxt.h \
    src/TodoMd.h \
    src/FileUtil.h \
    src/MdRichText.h

SOURCES = \
    src/main.cpp \
    src/MainWindow.cpp \
    src/MarkdownActions.cpp \
    src/MdEdit.cpp \
    src/MdView.cpp \
    src/TextPrompt.cpp \
    src/MdParser.cpp \
    src/TodoTxt.cpp \
    src/TodoMd.cpp \
    src/FileUtil.cpp \
    src/MdRichText.c \
    vendor/md4c/md4c.c \
    vendor/md4c/entity.c

INCLUDEPATH += $(QPEDIR)/include vendor/md4c
LIBS += -lqpe
