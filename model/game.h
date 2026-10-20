#ifndef GAME_H
#define GAME_H

#include "land.h"
#include "player.h"
#include "question.h"

#include <array>
#include <random>
#include <string>
#include <vector>

// 棋盘格数（7x7 棋盘的环形外围共 24 格：1 起点 + 15 普通地块 + 8 晋剧文化地块）
const int kBoardSize = 24;
const int kJinjuLandCount = 8;   // 晋剧文化地块数量（四类题目各占 2 块）
const int kNormalLandCount = 15; // 普通地块数量
const int kJinjuPerCategory = 2; // 每个题目类别对应的晋剧地块数

// 落地事件类型
enum class EventType
{
    None,         // 无事件
    StartReward,  // 停在起点
    UnownedLand,  // 无人拥有的空地，可购买
    OwnLand,      // 踩到自己的地产
    PayRent,      // 向其他玩家支付租金
    JinjuQuiz,    // 晋剧文化地块，需要答题
    Bankrupt      // 因付不起费用而破产
};

// 落地结算结果，供界面层展示与决策
struct LandEvent
{
    EventType type = EventType::None;
    int player = -1;             // 触发事件的玩家
    int land = -1;               // 触发事件的地块
    int owner = -1;              // 地块拥有者（-1 表示没有）
    int amount = 0;              // 对当前玩家造成的金钱变化
    bool needQuiz = false;       // 是否需要弹出答题窗口
    bool canBuy = false;         // 当前是否可以购买该地块
    bool playerBankrupt = false; // 玩家是否因此破产
    std::string text;            // 面向玩家的说明文字
};

// 答题结果
struct QuizResult
{
    bool correct = false;  // 是否答对
    int amount = 0;        // 金钱变化量
    bool bankrupt = false; // 是否因答错而破产
    std::string text;      // 面向玩家的说明文字
};

// 创建初始棋盘（24 个地块，包含 9 个晋剧文化地块）
std::vector<Land> createBoard();

// 游戏核心逻辑：只依赖 C++ 标准库，方便单元测试
class Game
{
public:
    // 使用 constexpr：C++17 下隐含 inline，无需在 .cpp 中重复定义
    // 经济参数经过对局模拟调优，目标是 4 人局约 10~15 分钟能分出胜负
    static constexpr int kInitialMoney = 3000; // 初始金钱
    static constexpr int kStartSalary = 100;   // 经过起点补贴
    static constexpr int kQuizReward = 100;     // 答对奖励
    static constexpr int kQuizPenalty = 150;   // 答错惩罚
    static constexpr int kTurnCost = 25;       // 每回合戏班开销（场地、行头、人工）
    static constexpr int kTurnCostGrowth = 5;  // 每 10 回合开销递增的金额
    static constexpr int kTurnCostMax = 120;   // 开销上限

    // 连片地产加成：地主每多持有一块地，名下所有地块的租金 +25%，
    // 这是让一局能在 10~15 分钟内分出胜负的关键（也更有博弈感）。
    static constexpr int kRentBonusPerProperty = 25;

    Game();

    // 开局：2~4 名玩家，names 可为空（使用默认玩家名）
    void start(int playerCount, const std::vector<std::string> &names = std::vector<std::string>());
    void reset();
    void setSeed(unsigned int seed); // 设置随机种子，便于复现

    // 数据访问
    int playerCount() const { return static_cast<int>(m_players.size()); }
    const Player &player(int index) const { return m_players.at(index); }
    Player &playerAt(int index) { return m_players.at(index); }
    const std::vector<Land> &lands() const { return m_lands; }
    int currentIndex() const { return m_current; }
    const Player &currentPlayer() const { return m_players.at(m_current); }
    int turnCount() const { return m_turn; }
    int lastDice() const { return m_lastDice; }

    bool isOver() const { return m_over; }
    int winnerIndex() const { return m_winner; }
    int aliveCount() const;

    // 掷骰子与移动（一格格走，方便界面做棋子移动动画）
    int rollDice();             // 掷出 1~6 点，并记录待走步数
    // 回合开始：当前玩家支付戏班开销；若因此破产返回 true
    bool beginTurn();
    bool hasPendingSteps() const { return m_pendingSteps > 0; }
    int pendingSteps() const { return m_pendingSteps; }
    bool moveOneStep();         // 走一格，返回是否经过起点
    int moveCurrent(int steps); // 一次走完，返回经过起点的次数

    // 落地结算
    LandEvent settleLand();
    bool canBuyCurrentLand() const;
    bool buyCurrentLand();
    int currentRent(int landIndex) const; // 计入连片加成后的实际租金
    int turnCost() const;                 // 本回合的戏班开销（随回合数递增）

    // 晋剧答题
    QuizCategory quizCategoryOf(int landIndex) const; // 某个晋剧地块所属的题目类别
    QuizCategory currentQuizCategory() const;         // 当前玩家所在地块的题目类别
    Question drawQuestionFor(int landIndex);          // 按地块类别抽题，同一类内不重复
    QuizResult submitQuizAnswer(int choice); // 提交答案，结算奖惩
    bool isQuizOpen() const { return m_quizOpen; }

    // 回合控制
    void endTurn();
    void forceBankrupt(int playerIndex);

private:
    void bankruptPlayer(int playerIndex);
    void refillBag(QuizCategory category);
    void assignQuizCategories();
    void checkGameOver();

    std::vector<Player> m_players; // 玩家
    std::vector<Land> m_lands;     // 棋盘
    std::vector<QuizCategory> m_landCategories;              // 每个晋剧地块对应的题目类别
    std::array<std::vector<Question>, kQuizCategoryCount> m_bags; // 四类各自的题袋
    Question m_lastQuestion;       // 最近抽出的题目
    std::mt19937 m_rng;            // 随机数引擎
    int m_current = 0;             // 当前玩家下标
    int m_lastDice = 0;            // 最近一次骰子点数
    int m_pendingSteps = 0;        // 待走步数
    int m_turn = 1;                // 当前回合数
    int m_winner = -1;             // 获胜者下标
    bool m_over = false;           // 游戏是否结束
    bool m_quizOpen = false;       // 是否有一道题等待提交
};

#endif // GAME_H
