# 《晋曲棋旅》晋剧文化主题大富翁 —— Qt 6 + C++17 + qmake

QT += core gui widgets

# 若安装了 Qt Multimedia 模块则启用（可播放 mp3/wav）；
# 未安装时程序自动回退到系统播放器（只支持 WAV），不影响运行。
qtHaveModule(multimedia) {
    QT += multimedia
    DEFINES += HAVE_QT_MULTIMEDIA
}

win32: LIBS += -lwinmm

CONFIG += c++17 warn_on
TEMPLATE = app
TARGET = JinQuQiLv

# 说明：工程内部的头文件一律使用相对路径包含（如 "../model/game.h"），
# 因此这里不添加 INCLUDEPATH —— 编译命令中不会出现含中文的绝对路径，
# 从根本上避免 MinGW 在非 ASCII 工程路径下找不到头文件的问题。

SOURCES += \
    main.cpp \
    model/player.cpp \
    model/land.cpp \
    model/game.cpp \
    model/questionbank.cpp \
    ui/mainwindow.cpp \
    ui/boardwidget.cpp \
    ui/questiondialog.cpp \
    ui/landdialog.cpp \
    ui/artwork.cpp \
    ui/audioplayer.cpp

HEADERS += \
    model/player.h \
    model/land.h \
    model/question.h \
    model/questionbank.h \
    model/game.h \
    ui/mainwindow.h \
    ui/boardwidget.h \
    ui/questiondialog.h \
    ui/landdialog.h \
    ui/artwork.h \
    ui/audioplayer.h

FORMS += \
    ui/mainwindow.ui

# MSVC 也需要按 UTF-8 解析源码中的中文字符串
msvc: QMAKE_CXXFLAGS += /utf-8

# 说明：资源（背景图/地标照片/戏曲图片/音频）全部在运行时自动查找，
# 工程里不做任何构建期复制，避免中文路径下 xcopy/make 报错。
