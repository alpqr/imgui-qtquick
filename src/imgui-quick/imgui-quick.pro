TEMPLATE = lib
TARGET = imgui-quick
BINDIR = $$OUT_PWD/../../bin
DESTDIR = $$BINDIR

CONFIG += staticlib
QT += quick

INCLUDEPATH += ../3rdparty/imgui

SOURCES += \
    ../3rdparty/imgui/imgui.cpp \
    ../3rdparty/imgui/imgui_draw.cpp \
    ../3rdparty/imgui/imgui_demo.cpp \
    imguiitem.cpp

HEADERS += \
    imguiitem.h
