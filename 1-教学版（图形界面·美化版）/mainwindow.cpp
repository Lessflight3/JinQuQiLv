#include "mainwindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>

// ============================================================
//  16 个格子在 5x5 网格里的行列位置
//  从 0 号（左下角）开始顺时针绕一圈
//  知识点：const 数组、下标与坐标的对应关系
// ============================================================
static const int kRow[16] = {4, 4, 4, 4, 4,   // 底边（0~4）：从左到右
                             3, 2, 1, 0,      // 右边（5~8）：从下到上
                             0, 0, 0, 0,      // 顶边（9~12）：从右到左
                             1, 2, 3};        // 左边（13~15）：从上到下
static const int kCol[16] = {0, 1, 2, 3, 4,
                             4, 4, 4, 4,
                             3, 2, 1, 0,
                             0, 0, 0};

// ============================================================
//  晋剧题库：5 道题
//  知识点：结构体数组的初始化写法
//  最后那个数字是正确答案：0=A，1=B，2=C，3=D
// ============================================================
static Quiz g_quiz[5] = {
    {"晋剧又被称为下列哪一个名称？", "中路梆子", "河南梆子", "河北梆子", "蒲剧", 0},
    {"晋剧主要流传于我国哪个省份？", "河南省", "山西省", "山东省", "安徽省", 1},
    {"《打金枝》是哪个剧种的代表剧目？", "越剧", "黄梅戏", "晋剧", "昆曲", 2},
    {"晋剧的主奏乐器是？", "晋胡（呼胡）", "京胡", "马头琴", "唢呐", 0},
    {"晋剧里“水袖”是哪个部位的技巧？", "头饰", "服饰袖口", "靴子", "脸谱", 1},
};

// ============================================================
//  界面配色（都写在一起，想换风格只改这几行）
//  知识点：Qt 用“样式表”(setStyleSheet) 美化界面，写法很像网页的 CSS
// ============================================================
static const char *kColorPaper  = "#fdf8ef";  // 背景：米白
static const char *kColorRed    = "#a83a2c";  // 主色：赭红
static const char *kColorGold   = "#c9a227";  // 点缀：金
static const char *kColorText   = "#40342a";  // 正文
static const char *kPlayerHex[2] = {"#d64541", "#2b7cd3"};  // 两位玩家的代表色

// ============================================================
//  构造函数：程序启动时自动执行
// ============================================================
MainWindow::MainWindow()
{
    m_current = 0;   // 从玩家一开始
    m_over = false;  // 游戏还没结束

    initBoard();     // 第一步：准备棋盘和玩家数据
    initUI();        // 第二步：搭界面
    refresh();       // 第三步：把数据显示出来
}

// ============================================================
//  初始化棋盘数据（16 个格子 + 两位玩家）
// ============================================================
void MainWindow::initBoard()
{
    // 知识点：给数组元素赋值，用 “类名(参数)” 构造一个新对象
    m_land[0] = Land("起点", TYPE_START, 0, 0);
    m_land[1] = Land("晋祠", TYPE_NORMAL, 200, 100);
    m_land[2] = Land("脸谱阁", TYPE_QUIZ, 0, 0);
    m_land[3] = Land("乔家大院", TYPE_NORMAL, 180, 90);
    m_land[4] = Land("平遥古城", TYPE_NORMAL, 160, 80);
    m_land[5] = Land("壶口瀑布", TYPE_NORMAL, 240, 120);
    m_land[6] = Land("梆子腔", TYPE_QUIZ, 0, 0);
    m_land[7] = Land("云冈石窟", TYPE_NORMAL, 220, 110);
    m_land[8] = Land("五台山", TYPE_NORMAL, 200, 100);
    m_land[9] = Land("水袖台", TYPE_QUIZ, 0, 0);
    m_land[10] = Land("雁门关", TYPE_NORMAL, 260, 130);
    m_land[11] = Land("悬空寺", TYPE_NORMAL, 180, 90);
    m_land[12] = Land("老陈醋坊", TYPE_NORMAL, 140, 70);
    m_land[13] = Land("洪洞大槐树", TYPE_NORMAL, 120, 60);
    m_land[14] = Land("八泉峡", TYPE_NORMAL, 180, 90);
    m_land[15] = Land("娘子关", TYPE_NORMAL, 200, 100);

    // 两位玩家，每人 1000 元
    m_player[0] = Player("玩家一", 1000);
    m_player[1] = Player("玩家二", 1000);
}

// ============================================================
//  创建界面（全部用代码写，不用 Qt Designer，方便对照学习）
// ============================================================
void MainWindow::initUI()
{
    setWindowTitle("晋曲棋旅 · 教学简化版");
    resize(1040, 700);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    // 整个窗口的底色 + 文字颜色（知识点：样式表）
    central->setStyleSheet(QString("background:%1; color:%2;").arg(kColorPaper, kColorText));

    // ---------- 顶部标题 ----------
    QLabel *title = new QLabel("晋 曲 棋 旅");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QString("font-size:30px; font-weight:bold; color:%1;"
                                 "letter-spacing:8px; padding:6px 0 0 0;").arg(kColorRed));

    QLabel *subtitle = new QLabel("晋剧文化主题大富翁 · 教学简化版　|　16 格 · 2 位玩家 · 买地收租 · 答晋剧题");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(QString("font-size:13px; color:%1; padding-bottom:8px;").arg(kColorGold));

    // ---------- 左边：棋盘 ----------
    // 知识点：QGridLayout 网格布局，addWidget(控件, 行, 列)
    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(10);          // 格子之间留 10 像素，看着不挤
    for (int i = 0; i < CELL_COUNT; i++)
    {
        // 用 QLabel 而不是按钮：标签可以显示彩色文字和圆角背景，更适合“看棋盘”
        m_cells[i] = new QLabel;
        m_cells[i]->setAlignment(Qt::AlignCenter);
        m_cells[i]->setMinimumSize(150, 86);
        grid->addWidget(m_cells[i], kRow[i], kCol[i]);
    }

    // 棋盘中间放一块说明牌（占 3x3 的区域）
    QLabel *center = new QLabel("晋 剧 文 化 地 块\n\n四 面 都 是 戏\n\n答对 +100 · 答错 −100");
    center->setAlignment(Qt::AlignCenter);
    center->setStyleSheet(QString("font-size:15px; color:%1; line-height:170%;"
                                  "border:2px dashed #e3d3ae; border-radius:16px;"
                                  "background:#fffdf7;").arg(kColorGold));
    grid->addWidget(center, 1, 1, 3, 3);

    // ---------- 右边：玩家卡片 + 骰子 + 按钮 + 日志 ----------
    QVBoxLayout *right = new QVBoxLayout;
    right->setSpacing(10);

    for (int i = 0; i < 2; i++)
    {
        m_card[i] = new QLabel;
        m_card[i]->setMinimumHeight(72);
        right->addWidget(m_card[i]);
    }

    m_labelDice = new QLabel("骰子点数：-");
    m_labelDice->setAlignment(Qt::AlignCenter);
    m_labelDice->setMinimumHeight(58);
    m_labelDice->setStyleSheet(QString("font-size:20px; font-weight:bold; color:%1;"
                                       "background:#fffdf7; border:2px solid %1; border-radius:14px;")
                                   .arg(kColorRed));

    QPushButton *btnRoll = new QPushButton("掷  骰  子");
    btnRoll->setMinimumHeight(52);
    btnRoll->setCursor(Qt::PointingHandCursor);
    // 知识点：用样式表给按钮做圆角、渐变和悬停变色
    btnRoll->setStyleSheet(QString(
        "QPushButton{font-size:20px; font-weight:bold; color:#fff; border:none; border-radius:14px;"
        "  background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #c04b3a, stop:1 #96301f);}"
        "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #d05a48, stop:1 #a83a26);}"
        "QPushButton:pressed{background:#8c2a1c;}"));

    m_log = new QTextEdit;
    m_log->setReadOnly(true);       // 日志只让程序写，玩家不能改
    m_log->setStyleSheet("font-size:13px; background:#fffdf7; border:1px solid #e3d3ae;"
                         "border-radius:12px; padding:6px; color:#5a4a38;");

    right->addWidget(m_labelDice);
    right->addWidget(btnRoll);
    right->addWidget(m_log);

    // ---------- 上下左右拼起来 ----------
    QHBoxLayout *middle = new QHBoxLayout;
    middle->addLayout(grid);
    middle->addLayout(right);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 10, 16, 14);
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);
    mainLayout->addLayout(middle);

    // 知识点：信号与槽 —— 点击按钮就会执行 rollDice() 函数
    connect(btnRoll, &QPushButton::clicked, this, &MainWindow::rollDice);
}

// ============================================================
//  刷新界面：把数据的变化显示到控件上
// ============================================================
void MainWindow::refresh()
{
    // 1) 两位玩家的信息卡片（用 HTML 让名字带颜色）
    for (int i = 0; i < 2; i++)
    {
        bool isTurn = (i == m_current && !m_over);
        QString html = QString("<div style='font-size:15px;'>"
                               "<b style='color:%1'>%2</b>%3<br>"
                               "<span style='color:#6b5b45;'>现金 <b>%4</b> 元　地产 <b>%5</b> 块</span>"
                               "</div>")
                           .arg(kPlayerHex[i], m_player[i].name(),
                                isTurn ? "　<b>← 该你了</b>" : "",
                                QString::number(m_player[i].money()),
                                QString::number(m_player[i].landCount()));
        if (m_player[i].isOut())
            html += "<div style='color:#9a8f80;'>已破产出局</div>";
        m_card[i]->setText(html);
        m_card[i]->setStyleSheet(QString(
            "background:#fffdf7; border-radius:12px; padding:8px 12px; border:2px solid %1;")
            .arg(isTurn ? kPlayerHex[i] : "#e3d3ae"));
    }

    // 2) 每个格子显示：名字 + 价格或地主 + 站在这里的玩家
    for (int i = 0; i < CELL_COUNT; i++)
    {
        QString html = QString("<div style='font-size:15px; font-weight:bold;'>%1</div>")
                           .arg(m_land[i].name());

        if (m_land[i].type() == TYPE_NORMAL)
        {
            if (m_land[i].hasOwner())
                html += QString("<div style='font-size:12px; color:%1;'>%2 的地</div>")
                            .arg(kPlayerHex[m_land[i].owner()],
                                 m_player[m_land[i].owner()].name());
            else
                html += QString("<div style='font-size:12px; color:#8a7b66;'>%1 元</div>")
                            .arg(m_land[i].price());
        }
        else if (m_land[i].type() == TYPE_QUIZ)
        {
            html += "<div style='font-size:12px; color:#7e57c2;'>晋剧答题</div>";
        }

        // 有玩家站在这里，就画一个彩色圆点 + 编号
        QString tokens;
        for (int p = 0; p < 2; p++)
        {
            if (!m_player[p].isOut() && m_player[p].pos() == i)
                tokens += QString("<span style='color:%1; font-size:17px;'>●</span>"
                                  "<span style='font-size:12px; color:%1;'>%2</span> ")
                              .arg(kPlayerHex[p], QString::number(p + 1));
        }
        if (!tokens.isEmpty())
            html += "<div>" + tokens + "</div>";
        m_cells[i]->setText(html);

        // 格子底色：起点金色、答题紫色、被买走用玩家色，其余米白
        QString bg = "#fffdf7";
        QString border = "#e3d3ae";
        if (m_land[i].owner() == 0)      { bg = "#fdecec"; border = kPlayerHex[0]; }
        else if (m_land[i].owner() == 1) { bg = "#eaf2fd"; border = kPlayerHex[1]; }
        else if (m_land[i].type() == TYPE_START) { bg = "#fff6d6"; border = kColorGold; }
        else if (m_land[i].type() == TYPE_QUIZ)  { bg = "#f3ecff"; border = "#b39ddb"; }

        m_cells[i]->setStyleSheet(QString("background:%1; border:2px solid %2; border-radius:12px;"
                                          "padding:4px;").arg(bg, border));
    }
}

// ============================================================
//  往日志框里加一行字
// ============================================================
void MainWindow::showMessage(QString text)
{
    m_log->append(text);
}

// ============================================================
//  掷骰子：一个完整回合的流程
// ============================================================
void MainWindow::rollDice()
{
    if (m_over)
        return;

    // 知识点：引用（&）—— 改 cur 就等于改 m_player[m_current]
    Player &cur = m_player[m_current];
    if (cur.isOut())
    {
        nextPlayer();
        return;
    }

    // 1) 掷骰子：1~6 点
    int dice = randomInt(1, 6);
    m_labelDice->setText("骰子点数：" + QString::number(dice));
    showMessage("【" + cur.name() + "】掷出 " + QString::number(dice) + " 点");

    // 2) 移动：知识点——取模运算 % 让位置在 0~15 之间循环
    int newPos = (cur.pos() + dice) % CELL_COUNT;
    cur.setPos(newPos);
    showMessage("        走到【" + m_land[newPos].name() + "】");

    // 3) 处理走到的地方会发生什么
    doLandEvent();

    // 4) 看看有没有人破产；没结束就换人
    if (!checkOver())
        nextPlayer();

    refresh();
}

// ============================================================
//  落地事件：起点 / 普通地块 / 答题地块
// ============================================================
void MainWindow::doLandEvent()
{
    Player &cur = m_player[m_current];
    Land &land = m_land[cur.pos()];

    if (land.type() == TYPE_START)
    {
        cur.addMoney(100);
        showMessage("        停在起点，领取 100 元");
    }
    else if (land.type() == TYPE_QUIZ)
    {
        askQuiz();      // 答题地块 → 抽一道题
    }
    else
    {
        // 普通地块的三种情况
        if (!land.hasOwner())
        {
            if (cur.money() >= land.price())
            {
                // 弹窗询问是否购买（知识点：QMessageBox）
                int answer = QMessageBox::question(this, "买地",
                        land.name() + "  价格 " + QString::number(land.price()) + " 元\n\n要买下吗？");
                if (answer == QMessageBox::Yes)
                {
                    cur.addMoney(-land.price());
                    cur.addLand();
                    land.setOwner(m_current);
                    showMessage("        买下【" + land.name() + "】");
                }
                else
                {
                    showMessage("        没有买");
                }
            }
            else
            {
                showMessage("        钱不够，买不起");
            }
        }
        else if (land.owner() == m_current)
        {
            showMessage("        这是自己的地，什么也没发生");
        }
        else
        {
            int rent = land.rent();
            cur.addMoney(-rent);
            m_player[land.owner()].addMoney(rent);
            showMessage("        这是别人的地，付租金 " + QString::number(rent) + " 元给 "
                        + m_player[land.owner()].name());
        }
    }

    // 钱变成负数就算破产
    if (cur.money() < 0)
    {
        cur.setOut(true);
        showMessage("        " + cur.name() + " 钱不够，破产出局！");
    }
}

// ============================================================
//  晋剧答题：从题库里随机抽一道，用下拉框选答案
// ============================================================
void MainWindow::askQuiz()
{
    Player &cur = m_player[m_current];

    int index = randomInt(0, 4);   // 5 道题里随机抽一道
    Quiz q = g_quiz[index];

    // 知识点：QStringList + QInputDialog::getItem 做选择框
    QStringList items;
    items << "A. " + q.a << "B. " + q.b << "C. " + q.c << "D. " + q.d;

    bool ok = false;
    QString choice = QInputDialog::getItem(this, "晋剧文化考校", q.question, items, 0, false, &ok);

    int myAnswer = -1;
    if (ok)
        myAnswer = items.indexOf(choice);   // 选的选项排在第几个（A=0，B=1……）

    if (myAnswer == q.answer)
    {
        cur.addMoney(100);
        showMessage("        答对了！奖励 100 元");
        QMessageBox::information(this, "答题结果", "回答正确，奖励 100 元！");
    }
    else
    {
        cur.addMoney(-100);
        QString right = QString("ABCD")[q.answer];   // 把 0~3 变成 A~D
        showMessage("        答错了，扣 100 元（正确答案：" + right + "）");
        QMessageBox::warning(this, "答题结果",
                             "回答错误，扣 100 元。\n正确答案是：" + right);
    }
}

// ============================================================
//  换下一位玩家（0 和 1 之间来回切换）
// ============================================================
void MainWindow::nextPlayer()
{
    m_current = 1 - m_current;    // 0 变 1，1 变 0
    showMessage("———— 轮到 " + m_player[m_current].name() + " ————");
}

// ============================================================
//  检查游戏是否结束（只剩一个人没破产）
// ============================================================
bool MainWindow::checkOver()
{
    if (m_over)
        return true;

    int alive = 0;      // 还有几个人没破产
    int who = 0;        // 最后活着的人是谁
    for (int i = 0; i < 2; i++)
    {
        if (!m_player[i].isOut())
        {
            alive = alive + 1;
            who = i;
        }
    }

    if (alive <= 1)
    {
        m_over = true;
        QString text = (alive == 1) ? (m_player[who].name() + " 获胜！") : "双方都破产了";
        showMessage("游戏结束：" + text);
        QMessageBox::information(this, "游戏结束", text);
        return true;
    }
    return false;
}

// ============================================================
//  生成 [low, high] 之间的随机整数
//  知识点：QRandomGenerator 是 Qt 提供的随机数工具
// ============================================================
int MainWindow::randomInt(int low, int high)
{
    return QRandomGenerator::global()->bounded(low, high + 1);
}
