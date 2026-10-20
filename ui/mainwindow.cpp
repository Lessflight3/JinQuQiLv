#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "artwork.h"
#include "audioplayer.h"
#include "boardwidget.h"
#include "landdialog.h"
#include "questiondialog.h"

#include "../model/questionbank.h"

#include <QAbstractButton>
#include <QAction>
#include <QDir>
#include <QFont>
#include <QFontMetrics>
#include <QFile>
#include <QGuiApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QScrollBar>
#include <QStringList>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

namespace {

// 四名玩家的棋子颜色
const QColor kPlayerColors[4] = {QColor(214, 62, 54), QColor(46, 118, 190), QColor(46, 148, 92),
                                 QColor(226, 142, 32)};

const char *kDefaultNames[4] = {"玩家一", "玩家二", "玩家三", "玩家四"};

const char *kBackgroundAudioKey = "xiaokaimen"; // 背景音乐：晋剧曲牌《小开门》

QString colorForEvent(EventType type)
{
    switch (type) {
    case EventType::JinjuQuiz:
        return QStringLiteral("#C9A0FF");
    case EventType::PayRent:
        return QStringLiteral("#FFB74D");
    case EventType::Bankrupt:
        return QStringLiteral("#FF7043");
    case EventType::UnownedLand:
        return QStringLiteral("#7ED6C4");
    default:
        return QStringLiteral("#D8D2C4");
    }
}

} // namespace

MainWindow::MainWindow(QWidget *parent, int autoPlayers, bool selfCheck, bool noMusic)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_musicEnabled(!noMusic)
    , m_autoPlayers(autoPlayers)
    , m_selfCheck(selfCheck)
{
    ui->setupUi(this);
    setupUi();
    startBackgroundMusic();

    if (m_autoPlayers > 0)
        startNewGame(); // 命令行直接开局

    if (m_selfCheck)
        runLayoutSelfCheck();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    // 棋盘控件占满左侧
    m_board = new BoardWidget(this);
    ui->boardLayout->addWidget(m_board);
    ui->mainLayout->setStretch(0, 1);
    ui->mainLayout->setStretch(1, 0);
    // 间距放大一档：棋盘与面板之间、面板内各控件之间都留出呼吸空间
    ui->mainLayout->setSpacing(18);
    ui->sidePanelLayout->setSpacing(12);
    ui->sidePanelLayout->setContentsMargins(4, 4, 4, 4);
    ui->sidePanelLayout->setStretchFactor(ui->tablePlayers, 2);
    ui->sidePanelLayout->setStretchFactor(ui->textLog, 3);
    // 面板内容变松散后，表格与日志的固定最小高度相应收一点，避免小屏被撑破
    ui->tablePlayers->setMinimumHeight(100);
    ui->textLog->setMinimumHeight(120);
    // 几个小按钮统一给一点高度，点起来也更舒服
    ui->btnNewGame->setMinimumHeight(34);
    ui->btnRules->setMinimumHeight(34);
    ui->btnMusic->setMinimumHeight(34);

    // 玩家信息表
    ui->tablePlayers->setColumnCount(4);
    ui->tablePlayers->setHorizontalHeaderLabels(QStringList{
        QStringLiteral("玩家"), QStringLiteral("现金"), QStringLiteral("地产"), QStringLiteral("状态")});
    ui->tablePlayers->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tablePlayers->verticalHeader()->setVisible(false);
    ui->tablePlayers->setShowGrid(false);
    ui->tablePlayers->setWordWrap(true);

    // 整体配色：深色 + 金色描边，保证在背景图上的可读性
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#centralwidget { background: transparent; }
        QLabel { color: #F3EADA; font-size: 15px; }
        QLabel#lblTitle { color: #F5D98C; font-size: 30px; font-weight: bold; }
        QLabel#lblSubtitle { color: #D9C7A0; font-size: 14px; }
        QLabel#lblTurn { color: #FFE0A3; font-size: 17px; font-weight: bold; }
        QLabel#lblCurrent { color: #FFFFFF; font-size: 16px; }
        QLabel#lblDice { color: #FFD98C; font-size: 16px; }
        QLabel#lblPlayers, QLabel#lblLog { color: #F0D98C; font-size: 16px; font-weight: bold; }
        QPushButton { color: #FFF6E5; background: rgba(120, 34, 34, 220); border: 1px solid #D4AF37;
                      border-radius: 8px; padding: 8px 10px; font-size: 15px; }
        QPushButton:hover { background: rgba(160, 48, 44, 235); }
        QPushButton:disabled { color: #9C9186; background: rgba(60, 44, 44, 200); border-color: #6B5A44; }
        QPushButton#btnRoll { font-size: 20px; font-weight: bold; background: rgba(158, 36, 32, 235); }
        QTableWidget { background: rgba(20, 15, 18, 205); color: #F2ECDF; font-size: 15px;
                       border: 1px solid #6B5A44; gridline-color: #4A4038; }
        QHeaderView::section { background: rgba(90, 26, 26, 230); color: #F5D98C;
                               border: 0px; padding: 5px; font-size: 14px; }
        QTextEdit { background: rgba(20, 15, 18, 205); color: #EDE7DA; font-size: 14px;
                    border: 1px solid #6B5A44; }
        QStatusBar { color: #E8DCC2; }
        QMenuBar { background: rgba(40, 24, 24, 230); color: #F0D98C; }
        QMenuBar::item:selected { background: rgba(120, 34, 34, 235); }
        QMenu { background: rgba(40, 24, 24, 245); color: #F2ECDF; }
        QToolTip { background: #2A1C1C; color: #F7EFDD; border: 1px solid #D4AF37; }
    )"));

    m_moveTimer = new QTimer(this);
    m_moveTimer->setInterval(170);
    connect(m_moveTimer, &QTimer::timeout, this, &MainWindow::onMoveStep);

    m_audio = new AudioPlayer(this);

    connect(ui->btnRoll, &QPushButton::clicked, this, &MainWindow::onRollDice);
    connect(ui->btnNewGame, &QPushButton::clicked, this, &MainWindow::startNewGame);
    connect(ui->btnRules, &QPushButton::clicked, this, &MainWindow::onShowRules);
    connect(ui->btnMusic, &QPushButton::clicked, this, &MainWindow::onToggleMusic);
    connect(ui->actionNewGame, &QAction::triggered, this, &MainWindow::startNewGame);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionRules, &QAction::triggered, this, &MainWindow::onShowRules);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onShowAbout);

    // 键盘快捷键：空格/回车掷骰子，M 切换配乐（演示时更方便）
    auto *rollShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(rollShortcut, &QShortcut::activated, this, &MainWindow::onRollDice);
    auto *rollShortcut2 = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(rollShortcut2, &QShortcut::activated, this, &MainWindow::onRollDice);
    auto *musicShortcut = new QShortcut(QKeySequence(Qt::Key_M), this);
    connect(musicShortcut, &QShortcut::activated, this, &MainWindow::onToggleMusic);

    applyScreenAdaptiveSize();
}

void MainWindow::applyScreenAdaptiveSize()
{
    // 依据屏幕可用区域决定窗口尺寸：既足够大气，又不会超出屏幕导致底部被裁掉
    const QScreen *scr = screen() ? screen() : QGuiApplication::primaryScreen();
    if (!scr)
        return;
    const QRect avail = scr->availableGeometry();
    const int wantW = qBound(1080, avail.width() - 60, 1640);
    const int wantH = qBound(660, avail.height() - 60, 1000);
    resize(wantW, wantH);

    // 面板宽度随窗口宽度自适应
    const int panelMin = qBound(300, wantW / 4, 400);
    ui->sidePanel->setMinimumWidth(panelMin);
    ui->sidePanel->setMaximumWidth(panelMin + 80);
}

void MainWindow::startBackgroundMusic()
{
    if (!m_musicEnabled) {
        ui->btnMusic->setText(QStringLiteral("配乐：关"));
        return;
    }
    const bool ok = m_audio && m_audio->playBackground(kBackgroundAudioKey);
    ui->btnMusic->setText(ok ? QStringLiteral("配乐：开") : QStringLiteral("配乐：关"));
    if (!ok && !m_musicHinted) {
        m_musicHinted = true;
        appendLog(QStringLiteral("未找到晋剧配乐《小开门》。请把音频文件放到："));
        appendLog(QStringLiteral("    %1%2.wav（或 .mp3）")
                      .arg(AudioPlayer::audioFolderHint())
                      .arg(QString::fromLatin1(kBackgroundAudioKey)),
                  QStringLiteral("#FFB74D"));
        appendLog(QStringLiteral("放好后重新运行程序，即可在游戏过程中循环播放晋剧配乐。"),
                  QStringLiteral("#FFB74D"));
    }
}

QColor MainWindow::playerColor(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex > 3)
        return QColor(150, 150, 150);
    return kPlayerColors[playerIndex];
}

QString MainWindow::nameOf(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= m_game.playerCount())
        return QStringLiteral("—");
    return QString::fromStdString(m_game.player(playerIndex).name());
}

void MainWindow::startNewGame()
{
    int count = m_autoPlayers;
    std::vector<std::string> names;

    if (m_autoPlayers > 0) {
        for (int i = 0; i < m_autoPlayers; ++i)
            names.push_back(kDefaultNames[i]);
    } else {
        bool ok = false;
        count = QInputDialog::getInt(this,
                                     QStringLiteral("开始新游戏"),
                                     QStringLiteral("请选择玩家数量（2~4 人）："),
                                     2, 2, 4, 1, &ok);
        if (!ok)
            return;
        for (int i = 0; i < count; ++i) {
            const QString defaultName = QString::fromUtf8(kDefaultNames[i]);
            bool okName = false;
            const QString name =
                QInputDialog::getText(this,
                                      QStringLiteral("玩家名称"),
                                      QStringLiteral("请输入第 %1 位玩家的名字：").arg(i + 1),
                                      QLineEdit::Normal,
                                      defaultName,
                                      &okName);
            names.push_back((okName && !name.trimmed().isEmpty()) ? name.trimmed().toStdString()
                                                                  : defaultName.toStdString());
        }
    }

    m_moveTimer->stop();
    m_busy = false;
    m_overAnnounced = false;
    m_game.start(count, names);

    ui->textLog->clear();
    appendLog(QStringLiteral("新对局开始：%1 位玩家，每人初始资金 %2 元，棋子全部位于起点。")
                  .arg(count)
                  .arg(Game::kInitialMoney),
              QStringLiteral("#8BC34A"));
    appendLog(QStringLiteral("棋盘共 24 格：15 个山西文化地标 + 8 个晋剧文化地块（四大类题目各占 2 块）。"),
              QStringLiteral("#8BC34A"));

    // 展示本局四类题目在 8 个晋剧地块上的分布
    QStringList distribution;
    for (int i = 0; i < static_cast<int>(m_game.lands().size()); ++i) {
        if (!m_game.lands()[i].isJinju())
            continue;
        const QString landName = QString::fromStdString(m_game.lands()[i].name());
        const QString cat = QString::fromUtf8(quizCategoryShort(m_game.quizCategoryOf(i)));
        distribution << QStringLiteral("%1(%2)").arg(landName, cat);
    }
    appendLog(QStringLiteral("本局晋剧题目分布：") + distribution.join(QStringLiteral("、")));

    refreshBoard();
    refreshPlayers();
    refreshStatus();
    setBusy(false);
    ui->statusbar->showMessage(QStringLiteral("轮到 %1 掷骰子").arg(nameOf(m_game.currentIndex())));
}

void MainWindow::onRollDice()
{
    if (m_busy || m_game.isOver() || m_game.playerCount() == 0)
        return;

    // 回合开始：先付戏班开销（场地、行头、人工），交不出就散班出局
    const QString who = nameOf(m_game.currentIndex());
    const int cost = m_game.turnCost();
    if (m_game.beginTurn()) {
        appendLog(QStringLiteral("【%1】交不出戏班开销 %2 元，散班出局！").arg(who).arg(cost),
                  QStringLiteral("#FF7043"));
        refreshBoard();
        refreshPlayers();
        refreshStatus();
        finishTurn();
        return;
    }
    appendLog(QStringLiteral("【%1】支付戏班开销 %2 元。").arg(who).arg(cost));
    refreshPlayers();
    refreshStatus();

    const int dice = m_game.rollDice();
    if (dice <= 0)
        return;

    appendLog(QStringLiteral("【%1】掷出 %2 点，前进 %2 步。")
                  .arg(nameOf(m_game.currentIndex()))
                  .arg(dice));
    ui->lblDice->setText(QStringLiteral("骰子点数：%1").arg(dice));
    setBusy(true);
    ui->statusbar->showMessage(QStringLiteral("%1 正在移动……").arg(nameOf(m_game.currentIndex())));
    m_moveTimer->start();
}

void MainWindow::onMoveStep()
{
    if (m_game.moveOneStep()) {
        appendLog(QStringLiteral("【%1】经过起点，领取补贴 %2 元。")
                      .arg(nameOf(m_game.currentIndex()))
                      .arg(Game::kStartSalary),
                  QStringLiteral("#64B5F6"));
    }
    refreshBoard();
    refreshPlayers();
    refreshStatus();

    if (!m_game.hasPendingSteps()) {
        m_moveTimer->stop();
        handleLanding();
    }
}

void MainWindow::handleLanding()
{
    const LandEvent event = m_game.settleLand();
    appendLog(QString::fromStdString(event.text), colorForEvent(event.type));
    refreshBoard();
    refreshPlayers();
    refreshStatus();

    if (m_game.isOver()) {
        finishTurn();
        return;
    }

    if (event.needQuiz) {
        handleQuiz(event.land);
        refreshBoard();
        refreshPlayers();
        refreshStatus();
    } else if (event.canBuy) {
        const Land &land = m_game.lands()[event.land];
        const QString landName = QString::fromStdString(land.name());
        const bool buy = LandDialog::askBuy(this, landName, QString::fromStdString(land.key()),
                                            land.price(), land.rent(),
                                            m_game.currentPlayer().money(),
                                            nameOf(m_game.currentIndex()));
        if (buy && m_game.buyCurrentLand()) {
            appendLog(QStringLiteral("【%1】买下【%2】，支付 %3 元。")
                          .arg(nameOf(m_game.currentIndex()))
                          .arg(landName)
                          .arg(land.price()),
                      QStringLiteral("#4DB6AC"));
            refreshBoard();
            refreshPlayers();
            refreshStatus();
        }
    } else if (event.type == EventType::PayRent) {
        // 收租时也展示该地标的地域风貌图
        const Land &land = m_game.lands()[event.land];
        const QString ownerName = (event.owner >= 0) ? nameOf(event.owner) : QStringLiteral("对手");
        const QStringList info = {
            QStringLiteral("这里是 <b>%1</b>").arg(ownerName),
            QStringLiteral("本次支付租金：<b>%1</b> 元").arg(-event.amount),
            QStringLiteral("%1 剩余现金：<b>%2</b> 元")
                .arg(nameOf(m_game.currentIndex()))
                .arg(m_game.currentPlayer().money()),
        };
        LandDialog::showInfo(this, QString::fromStdString(land.name()), QStringLiteral("踩到他人地产"),
                             QString::fromStdString(land.key()), info);
    }

    finishTurn();
}

void MainWindow::handleQuiz(int landIndex)
{
    const int playerIndex = m_game.currentIndex();
    const Question question = m_game.drawQuestionFor(landIndex);
    const QString landName = QString::fromStdString(m_game.lands()[landIndex].name());

    // 有题目音频时：暂停背景配乐《小开门》，只留题目声音；答完再恢复
    const bool hasAudio =
        (question.category == QuizCategory::Tune || question.category == QuizCategory::Aria)
        && !AudioPlayer::findAudio(QString::fromStdString(question.mediaKey)).isEmpty();
    if (hasAudio) {
        const qint64 pausePos = m_audio->backgroundPositionMs();
        m_audio->suspendBackground();
        appendLog(QStringLiteral("配乐《小开门》暂停在 %1 秒处；本题音频自动播放。")
                      .arg(pausePos / 1000.0, 0, 'f', 1),
                  QStringLiteral("#B39DDB"));
    }

    const int choice = QuestionDialog::ask(this, question, landName, m_audio);
    m_audio->stopEffect(); // 停掉题目音频
    if (hasAudio) {
        m_audio->resumeBackground(); // 恢复配乐
        appendLog(QStringLiteral("配乐《小开门》从 %1 秒处继续播放（不从头开始）。")
                      .arg(m_audio->backgroundPositionMs() / 1000.0, 0, 'f', 1),
                  QStringLiteral("#B39DDB"));
    }

    const QuizResult result = m_game.submitQuizAnswer(choice);

    const QString answerText =
        QStringLiteral("%1. %2")
            .arg(QChar('A' + question.answer))
            .arg(QString::fromStdString(question.options[static_cast<size_t>(question.answer)]));

    QString detail;
    if (choice < 0)
        detail += QStringLiteral("你放弃了作答，本题按答错处理。<br><br>");
    detail += QStringLiteral("正确答案：<b>%1</b><br><br>%2")
                  .arg(answerText, QString::fromStdString(result.text));
    if (!question.tip.empty())
        detail += QStringLiteral("<br><br><span style='color:#7E57C2;'>知识卡：%1</span>")
                      .arg(QString::fromStdString(question.tip));
    detail += QStringLiteral("<br><br>当前现金：<b>%1</b> 元")
                  .arg(m_game.player(playerIndex).money());

    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("答题结果 · %1")
                           .arg(QString::fromUtf8(quizCategoryName(question.category))));
    box.setIcon(result.correct ? QMessageBox::Information : QMessageBox::Warning);
    box.setText(result.correct ? QStringLiteral("回答正确！") : QStringLiteral("回答错误"));
    box.setInformativeText(detail);
    box.exec();

    appendLog(QStringLiteral("【%1题】%2")
                  .arg(QString::fromUtf8(quizCategoryShort(question.category)),
                       QString::fromStdString(result.text)),
              result.correct ? QStringLiteral("#8BC34A") : QStringLiteral("#FF7043"));
}

void MainWindow::finishTurn()
{
    if (!m_game.isOver())
        m_game.endTurn();

    refreshBoard();
    refreshPlayers();
    refreshStatus();
    setBusy(false);

    if (m_game.isOver())
        announceGameOver();
    else
        ui->statusbar->showMessage(QStringLiteral("轮到 %1 行动").arg(nameOf(m_game.currentIndex())));
}

void MainWindow::announceGameOver()
{
    if (m_overAnnounced)
        return;
    m_overAnnounced = true;

    const int winner = m_game.winnerIndex();
    if (winner >= 0) {
        const Player &p = m_game.player(winner);
        appendLog(QStringLiteral("游戏结束：%1 成为最终赢家！").arg(QString::fromStdString(p.name())),
                  QStringLiteral("#FFD54F"));
        QMessageBox::information(this,
                                 QStringLiteral("游戏结束"),
                                 QStringLiteral("🏆 %1 成为最后的赢家！\n\n剩余现金：%2 元\n持有地产：%3 块")
                                     .arg(QString::fromStdString(p.name()))
                                     .arg(p.money())
                                     .arg(p.propertyCount()));
    } else {
        appendLog(QStringLiteral("游戏结束：所有玩家均已破产。"), QStringLiteral("#FF7043"));
        QMessageBox::information(this, QStringLiteral("游戏结束"),
                                 QStringLiteral("所有玩家都已破产，本局无人获胜。"));
    }
    ui->btnRoll->setEnabled(false);
    ui->statusbar->showMessage(QStringLiteral("本局结束，可点击“开始新游戏”再来一局"));
}

void MainWindow::refreshBoard()
{
    if (m_game.playerCount() == 0)
        return;

    QList<BoardWidget::Token> tokens;
    QList<QColor> colors;
    QList<int> categories;
    QStringList names;

    for (int i = 0; i < static_cast<int>(m_game.lands().size()); ++i) {
        categories.append(m_game.lands()[i].isJinju()
                              ? static_cast<int>(m_game.quizCategoryOf(i))
                              : -1);
    }

    for (int i = 0; i < m_game.playerCount(); ++i) {
        const Player &p = m_game.player(i);
        colors.append(playerColor(i));
        names.append(QString::fromStdString(p.name()));
        if (p.isBankrupt())
            continue;
        BoardWidget::Token token;
        token.cell = p.position();
        token.color = playerColor(i);
        token.label = QString::number(i + 1);
        tokens.append(token);
    }

    m_board->setLands(m_game.lands());
    m_board->setQuizCategories(categories);
    QList<int> rents;
    for (int i = 0; i < static_cast<int>(m_game.lands().size()); ++i)
        rents.append(m_game.currentRent(i));
    m_board->setEffectiveRents(rents);
    m_board->setPlayerColors(colors);
    m_board->setPlayerNames(names);
    m_board->setTokens(tokens);
    m_board->setCurrentCell(m_game.currentPlayer().position());
}

void MainWindow::refreshPlayers()
{
    const int current = m_game.currentIndex();
    ui->tablePlayers->setRowCount(m_game.playerCount());
    for (int i = 0; i < m_game.playerCount(); ++i) {
        const Player &p = m_game.player(i);

        auto *nameItem = new QTableWidgetItem(QString::fromStdString(p.name()));
        nameItem->setForeground(playerColor(i));
        QFont nameFont = nameItem->font();
        nameFont.setBold(i == current && !m_game.isOver());
        nameItem->setFont(nameFont);
        ui->tablePlayers->setItem(i, 0, nameItem);

        ui->tablePlayers->setItem(i, 1, new QTableWidgetItem(QString::number(p.money())));
        ui->tablePlayers->setItem(i, 2, new QTableWidgetItem(QString::number(p.propertyCount())));

        QString state = QStringLiteral("等待");
        QColor stateColor(200, 192, 178);
        if (p.isBankrupt()) {
            state = QStringLiteral("已破产");
            stateColor = QColor(158, 158, 158);
        } else if (i == current && !m_game.isOver()) {
            state = QStringLiteral("行动中");
            stateColor = playerColor(i).lighter(130);
        }
        auto *stateItem = new QTableWidgetItem(state);
        stateItem->setForeground(stateColor);
        ui->tablePlayers->setItem(i, 3, stateItem);
    }
    ui->tablePlayers->resizeRowsToContents();
}

void MainWindow::refreshStatus()
{
    ui->lblTurn->setText(QStringLiteral("第 %1 回合 · 戏班开销 %2 元")
                             .arg(m_game.turnCount())
                             .arg(m_game.turnCost()));
    if (m_game.playerCount() == 0)
        return;
    const Player &cur = m_game.currentPlayer();
    ui->lblCurrent->setText(QStringLiteral("当前玩家：%1（现金 %2 元）")
                                .arg(QString::fromStdString(cur.name()))
                                .arg(cur.money()));
    ui->lblDice->setText(m_game.lastDice() > 0
                             ? QStringLiteral("骰子点数：%1").arg(m_game.lastDice())
                             : QStringLiteral("骰子点数：—"));
    ui->btnRoll->setEnabled(!m_busy && !m_game.isOver());
}

void MainWindow::appendLog(const QString &text, const QString &color)
{
    const QString line = QStringLiteral("· %1").arg(text.toHtmlEscaped());
    if (color.isEmpty())
        ui->textLog->append(line);
    else
        ui->textLog->append(QStringLiteral("<span style=\"color:%1;\">%2</span>").arg(color, line));
    ui->textLog->verticalScrollBar()->setValue(ui->textLog->verticalScrollBar()->maximum());
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    ui->btnRoll->setEnabled(!busy && !m_game.isOver());
    ui->btnNewGame->setEnabled(!busy);
    ui->actionNewGame->setEnabled(!busy);
}

void MainWindow::onToggleMusic()
{
    m_musicEnabled = !m_musicEnabled;
    if (m_musicEnabled) {
        const bool ok = m_audio->playBackground(kBackgroundAudioKey);
        ui->btnMusic->setText(ok ? QStringLiteral("配乐：开") : QStringLiteral("配乐：关"));
        if (!ok) {
            m_musicEnabled = false;
            appendLog(QStringLiteral("未能播放配乐，请确认 %1%2.wav 是否存在。")
                          .arg(AudioPlayer::audioFolderHint())
                          .arg(QString::fromLatin1(kBackgroundAudioKey)),
                      QStringLiteral("#FFB74D"));
        } else {
            appendLog(QStringLiteral("晋剧配乐《小开门》开始播放。"), QStringLiteral("#8BC34A"));
        }
    } else {
        m_audio->stopBackground();
        ui->btnMusic->setText(QStringLiteral("配乐：关"));
        appendLog(QStringLiteral("配乐已关闭。"));
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_background.size() != size() || m_background.isNull())
        m_background = Artwork::mainBackground(size());
    QPainter painter(this);
    painter.drawPixmap(0, 0, m_background);
}

void MainWindow::runLayoutSelfCheck()
{
    // 布局完成后检查是否有控件放不下文字（用于自动化排查“字被遮住”）
    QTimer::singleShot(900, this, [this]() {
        int checked = 0;
        int problems = 0;
        QStringList report;
        const QList<QWidget *> widgets = findChildren<QWidget *>();
        for (QWidget *w : widgets) {
            if (!w->isVisible() || w->isWindow())
                continue;
            const QString cls = QString::fromLatin1(w->metaObject()->className());
            if (cls.contains("QScrollBar") || cls.contains("QMenu") || cls.contains("QTip")
                || cls.contains("QHeaderView"))
                continue;
            ++checked;
            QSize need = w->minimumSizeHint();
            if (auto *label = qobject_cast<QLabel *>(w)) {
                if (!label->text().isEmpty() && label->wordWrap()) {
                    const QFontMetrics fm(label->font());
                    const QRect r = fm.boundingRect(QRect(0, 0, qMax(20, label->width()), 10000),
                                                    Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                                                    label->text());
                    need.setHeight(qMax(need.height(), r.height()));
                }
            }
            if (w->width() + 1 < need.width() || w->height() + 1 < need.height()) {
                ++problems;
                const QString line = QStringLiteral("LAYOUT_WARN %1 '%2' size=%3x%4 need=%5x%6")
                                         .arg(cls, w->objectName())
                                         .arg(w->width())
                                         .arg(w->height())
                                         .arg(need.width())
                                         .arg(need.height());
                report << line;
                std::printf("%s\n", qPrintable(line));
            }
        }
        const QString summary = QStringLiteral("SELFCHECK window=%1x%2 widgets=%3 problems=%4")
                                    .arg(width())
                                    .arg(height())
                                    .arg(checked)
                                    .arg(problems);
        report.prepend(summary);
        if (m_board)
            report << m_board->layoutReport();

        // 题目音频点名：一次性列出哪些题有音频、哪些还缺（便于补齐素材）
        QList<QString> keys;
        for (const Question &q : allQuestions()) {
            if (q.category != QuizCategory::Tune && q.category != QuizCategory::Aria)
                continue;
            const QString key = QString::fromStdString(q.mediaKey);
            if (!key.isEmpty() && !keys.contains(key))
                keys.append(key);
        }
        QStringList ready;
        QStringList missing;
        for (const QString &k : keys) {
            if (AudioPlayer::findAudio(k).isEmpty())
                missing << k;
            else
                ready << k;
        }
        report << QStringLiteral("AUDIO 题目音频就绪 %1 个：%2")
                      .arg(ready.size())
                      .arg(ready.join(QLatin1Char(' ')));
        report << QStringLiteral("AUDIO 题目音频缺失 %1 个：%2")
                      .arg(missing.size())
                      .arg(missing.join(QLatin1Char(' ')));
        std::printf("%s\n", qPrintable(summary));
        std::fflush(stdout);

        // GUI 程序没有控制台，因此同时写出报告文件，便于自动检查
        QFile file(QDir::currentPath() + "/layout_selfcheck.txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << report.join(QLatin1Char('\n')) << Qt::endl;
        }
    });
}

void MainWindow::onShowRules()
{
    const QString rules =
        QStringLiteral(
            "<h3>《晋曲棋旅》游戏规则</h3>"
            "<p><b>1. 开局：</b>2~4 名玩家，每人初始资金 %1 元，棋子停在起点。</p>"
            "<p><b>2. 戏班开销：</b>每回合开演前要支付戏班开销（场地、行头、人工），"
            "初始 30 元，每过 10 回合涨 5 元，最高 120 元；交不出就散班出局。</p>"
            "<p><b>3. 掷骰子：</b>掷出 1~6 点向前移动，经过起点领取 %2 元补贴。</p>"
            "<p><b>4. 普通地块（15 个山西文化地标）：</b>无人拥有时可购买；"
            "踩到对手的地产要付租金。地主的<b>连片地产</b>越多，租金越高"
            "（每多持有一块地，名下所有地块租金 +25%%）。</p>"
            "<p><b>5. 晋剧文化地块（8 个，四类题目各占 2 块）：</b>随机分为四类题目——"
            "<b>晋剧常识</b>、<b>听曲牌识曲牌</b>、<b>听唱段识剧目</b>、<b>看图识戏曲</b>，"
            "答对 +%3 元，答错 −%4 元。</p>"
            "<p><b>6. 破产：</b>现金不足以支付时出局，名下地产重新待售。</p>"
            "<p><b>7. 胜负：</b>场上只剩一名玩家时获胜。</p>")
            .arg(Game::kInitialMoney)
            .arg(Game::kStartSalary)
            .arg(Game::kQuizReward)
            .arg(Game::kQuizPenalty);
    QMessageBox::information(this, QStringLiteral("游戏规则"), rules);
}

void MainWindow::onShowAbout()
{
    QString audioState = QStringLiteral("背景音乐《小开门》");
    audioState += (m_audio && m_audio->isBackgroundPlaying()) ? QStringLiteral("：正在播放")
                                                             : QStringLiteral("：未播放（请放入音频文件）");
    QMessageBox::about(
        this, QStringLiteral("关于本作品"),
        QStringLiteral("<h3>晋曲棋旅</h3>"
                       "<p>晋剧文化主题大富翁 · Qt 课程设计</p>"
                       "<p>技术栈：Qt 6 + C++17 + qmake</p>"
                       "<p>界面背景、地块风貌插画、戏曲元素图片均由程序绘制；"
                       "把真实照片放入 resources/ 目录即可自动替换。</p>"
                       "<p>%1</p>")
            .arg(audioState));
}
