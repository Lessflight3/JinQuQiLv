// ============================================================
//  程序入口：每个 Qt 程序都从 main 函数开始
// ============================================================
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);   // 每个 Qt 程序都要先创建 QApplication

    MainWindow window;              // 创建主窗口（会调用 MainWindow 的构造函数）
    window.show();                  // 把窗口显示出来

    return app.exec();              // 进入事件循环，等待用户操作
}
