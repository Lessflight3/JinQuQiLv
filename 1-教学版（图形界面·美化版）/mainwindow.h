#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "land.h"
#include "player.h"
#include "quiz.h"

// Qt 的控件（用到的才包含）
class QLabel;
class QPushButton;
class QTextEdit;

// ============================================================
//  主窗口类：整个游戏都写在这里，方便初学者对照
//  知识点：继承 QMainWindow、信号与槽（槽函数写在 private slots 里）
// ============================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT   // 只要用到信号槽，这个宏必须写

public:
    MainWindow();

private slots:
    void rollDice();   // 点“掷骰子”按钮时执行

private:
    // ---- 游戏数据 ----
    static const int CELL_COUNT = 16;   // 一共 16 个格子
    Land m_land[CELL_COUNT];            // 棋盘：16 个格子
    Player m_player[2];                 // 两位玩家

    int m_current;          // 当前轮到谁（0 或 1）
    bool m_over;            // 游戏是否已结束
    QLabel *m_card[2];      // 两位玩家的信息卡片
    QLabel *m_labelDice;    // 显示骰子点数
    QLabel *m_cells[CELL_COUNT]; // 棋盘上的 16 个格子（用标签显示，方便显示彩色文字）
    QTextEdit *m_log;       // 日志框

    // ---- 内部用的函数 ----
    void initBoard();              // 初始化棋盘数据
    void initUI();                 // 创建界面
    void refresh();                // 刷新界面显示
    void showMessage(QString text);// 往日志里加一行
    void doLandEvent();            // 处理“走到哪个格子”
    void nextPlayer();             // 换下一位玩家
    void askQuiz();                // 抽一道晋剧题目来答
    bool checkOver();              // 检查是否结束（结束时弹窗提示）
    int randomInt(int low, int high);// 生成 [low, high] 的随机整数
};

#endif // MAINWINDOW_H
