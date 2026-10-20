# 《晋曲棋旅》教学简化版
# 只用最基础的 C++ 知识 + 最简单的 Qt 控件

QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = JinQuQiLvSimple

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h \
    player.h \
    land.h \
    quiz.h
