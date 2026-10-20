// 核心逻辑单元测试：不依赖 Qt，可直接用 g++ / MSVC 编译运行
// 编译示例：g++ -std=c++17 -I. tests/model_test.cpp model/*.cpp -o model_test

#include "../model/game.h"
#include "../model/land.h"
#include "../model/player.h"
#include "../model/questionbank.h"

#include <iostream>
#include <map>
#include <set>
#include <string>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#endif

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char *what, int line)
{
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  [FAIL] line " << line << ": " << what << "\n";
    }
}

#define CHECK(cond) check((cond), #cond, __LINE__)

void section(const char *title)
{
    std::cout << "[ RUN  ] " << title << "\n";
}

// 1. 玩家类：金钱增减、地产持有、破产标记
void testPlayerBasics()
{
    section("Player: money / properties / bankrupt flag");
    Player p("测试玩家", 1000);
    CHECK(p.money() == 1000);
    p.addMoney(-300);
    CHECK(p.money() == 700);
    p.addMoney(500);
    CHECK(p.money() == 1200);

    CHECK(p.propertyCount() == 0);
    p.addProperty(3);
    p.addProperty(3);
    p.addProperty(8);
    CHECK(p.propertyCount() == 2);
    CHECK(p.owns(3));
    CHECK(!p.owns(5));
    CHECK(!p.isBankrupt());
}

// 2. 棋盘数据：四六分（14 普通 / 9 晋剧）与素材键
void testBoardData()
{
    section("Board: 24 cells = 1 start + 15 normal + 8 jinju");
    const std::vector<Land> board = createBoard();
    CHECK(static_cast<int>(board.size()) == kBoardSize);
    CHECK(board.front().isStart());

    int jinju = 0;
    int normal = 0;
    std::set<std::string> keys;
    for (const Land &land : board) {
        if (land.isJinju())
            ++jinju;
        if (land.isNormal()) {
            ++normal;
            CHECK(land.price() > 0);
            CHECK(land.rent() > 0);
            CHECK(land.rent() < land.price());
            CHECK(!land.key().empty()); // 普通地块必须有素材键（对应地域风貌图）
            keys.insert(land.key());
        }
        CHECK(land.owner() == -1);
    }
    CHECK(jinju == kJinjuLandCount);
    CHECK(normal == kNormalLandCount);
    CHECK(static_cast<int>(keys.size()) == normal); // 每个地标一幅独立插画

    // 晋剧地块 8 个、普通地块 15 个（四类题目各占 2 块晋剧地块）
    const double ratio = double(jinju) / double(jinju + normal);
    CHECK(ratio > 0.30 && ratio < 0.45);
    CHECK(jinju == kQuizCategoryCount * kJinjuPerCategory);

    // 八泉峡：由原晋剧地块改为普通地块
    bool hasBaquanxia = false;
    for (const Land &land : board) {
        if (land.key() == "baquanxia") {
            hasBaquanxia = true;
            CHECK(land.isNormal());
        }
    }
    CHECK(hasBaquanxia);
}

// 3. 题库：四类各 20 道以上，字段合法
void testQuestionBank()
{
    section("Question bank: 4 categories, 20+ each, valid fields");
    for (int i = 0; i < kQuizCategoryCount; ++i) {
        const QuizCategory c = static_cast<QuizCategory>(i);
        const std::vector<Question> &list = questionsOf(c);
        std::cout << "         " << quizCategoryName(c) << ": " << list.size() << " 题\n";
        CHECK(static_cast<int>(list.size()) >= 20);
        for (const Question &q : list) {
            CHECK(!q.text.empty());
            CHECK(q.answer >= 0 && q.answer <= 3);
            for (const std::string &opt : q.options)
                CHECK(!opt.empty());
            CHECK(q.category == c);
            if (c != QuizCategory::CommonSense)
                CHECK(!q.mediaKey.empty()); // 曲牌/唱段/图片题都要有素材键
        }
    }
    CHECK(static_cast<int>(allQuestions().size()) >= kQuizCategoryCount * 20);
}

// 4. 骰子：范围 1~6，且六个点数都能掷出
void testDice()
{
    section("Dice: 1..6 range and coverage");
    Game game;
    game.setSeed(20261002u);
    game.start(2);
    std::set<int> values;
    for (int i = 0; i < 600; ++i) {
        const int dice = game.rollDice();
        CHECK(dice >= 1 && dice <= 6);
        values.insert(dice);
        while (game.hasPendingSteps())
            game.moveOneStep();
    }
    CHECK(values.size() == 6);
}

// 5. 移动与绕圈补贴
void testMoveAndSalary()
{
    section("Move: one lap gives start salary");
    Game game;
    game.setSeed(7u);
    game.start(2);
    const int before = game.currentPlayer().money();
    CHECK(game.moveCurrent(kBoardSize) == 1);
    CHECK(game.currentPlayer().position() == 0);
    CHECK(game.currentPlayer().money() == before + Game::kStartSalary);
}

// 6. 买地与收租
void testBuyAndRent()
{
    section("Buy land and pay rent");
    Game game;
    game.setSeed(11u);
    game.start(2);

    game.playerAt(0).setPosition(1);
    CHECK(game.canBuyCurrentLand());
    const int price = game.lands()[1].price();
    CHECK(game.buyCurrentLand());
    CHECK(game.lands()[1].owner() == 0);
    CHECK(game.player(0).money() == Game::kInitialMoney - price);
    CHECK(!game.canBuyCurrentLand());

    game.endTurn();
    game.playerAt(1).setPosition(1);
    const int rent = game.lands()[1].rent();
    const LandEvent ev = game.settleLand();
    CHECK(ev.type == EventType::PayRent);
    CHECK(ev.amount == -rent);
    CHECK(game.player(0).money() == Game::kInitialMoney - price + rent);
}

// 7. 破产：地产释放、只剩一人判胜
void testBankruptcyAndVictory()
{
    section("Bankruptcy releases lands, last survivor wins");
    Game game;
    game.setSeed(3u);
    game.start(3);

    game.playerAt(0).setPosition(1);
    CHECK(game.buyCurrentLand());
    const int price = game.lands()[1].price();
    const int rent = game.lands()[1].rent();

    game.endTurn();
    game.playerAt(1).setPosition(1);
    const int shortMoney = rent - 10; // 交不起租金
    game.playerAt(1).setMoney(shortMoney);
    const LandEvent ev = game.settleLand();
    CHECK(ev.type == EventType::Bankrupt);
    CHECK(game.player(1).isBankrupt());
    CHECK(game.player(1).money() == 0);
    CHECK(game.player(0).money() == Game::kInitialMoney - price + shortMoney);
    CHECK(game.aliveCount() == 2);
    CHECK(!game.isOver());

    // 破产玩家名下地产释放（3 号是普通地块）
    game.playerAt(1).setPosition(3);
    game.playerAt(1).setMoney(1000);
    game.playerAt(1).setBankrupt(false);
    CHECK(game.buyCurrentLand());
    CHECK(game.lands()[3].owner() == 1);
    game.forceBankrupt(1);
    CHECK(game.lands()[3].owner() == -1);
    CHECK(game.player(1).propertyCount() == 0);

    game.forceBankrupt(2);
    CHECK(game.isOver());
    CHECK(game.winnerIndex() == 0);
}

// 8. 回合切换跳过破产玩家
void testTurnSwitch()
{
    section("Turn switch skips bankrupt players");
    Game game;
    game.setSeed(5u);
    game.start(3);
    game.forceBankrupt(1);
    game.endTurn();
    CHECK(game.currentIndex() == 2);
    game.endTurn();
    CHECK(game.currentIndex() == 0);
    CHECK(game.turnCount() == 3);
}

// 9. 晋剧地块类别：8 个地块，四大类各占 2 块
void testQuizCategoryAssignment()
{
    section("Quiz categories: 2 lands per category");
    Game game;
    game.setSeed(1234u);
    game.start(2);

    std::map<int, int> countByCategory;
    int jinjuCount = 0;
    for (int i = 0; i < static_cast<int>(game.lands().size()); ++i) {
        if (!game.lands()[i].isJinju())
            continue;
        ++jinjuCount;
        const int c = static_cast<int>(game.quizCategoryOf(i));
        CHECK(c >= 0 && c < kQuizCategoryCount);
        countByCategory[c]++;
    }
    CHECK(jinjuCount == kJinjuLandCount);
    CHECK(static_cast<int>(countByCategory.size()) == kQuizCategoryCount);
    for (int i = 0; i < kQuizCategoryCount; ++i)
        CHECK(countByCategory[i] == kJinjuPerCategory); // 每类恰好 2 块

    // 换一个种子，分类分布通常会变化（说明是随机散布）
    Game other;
    other.setSeed(4321u);
    other.start(2);
    bool different = false;
    for (int i = 0; i < static_cast<int>(game.lands().size()); ++i) {
        if (game.lands()[i].isJinju()
            && game.quizCategoryOf(i) != other.quizCategoryOf(i))
            different = true;
    }
    CHECK(different);
}

// 10. 抽题：同一类一轮内不重复；答对奖励、答错惩罚；不能重复提交
void testQuizDrawAndScore()
{
    section("Quiz: no repeat in one round, reward / penalty");
    Game game;
    game.setSeed(99u);
    game.start(2);

    // 找一个晋剧地块，按它的类别连续抽题
    int jinjuLand = -1;
    for (int i = 0; i < static_cast<int>(game.lands().size()); ++i) {
        if (game.lands()[i].isJinju()) {
            jinjuLand = i;
            break;
        }
    }
    CHECK(jinjuLand >= 0);
    const QuizCategory category = game.quizCategoryOf(jinjuLand);
    const int total = questionCountOf(category);
    CHECK(total >= 20);

    std::set<std::string> seen;
    for (int i = 0; i < total; ++i) {
        const Question q = game.drawQuestionFor(jinjuLand);
        CHECK(!q.text.empty());
        CHECK(q.category == category);
        CHECK(game.isQuizOpen());
        seen.insert(q.text);
        const int money = game.currentPlayer().money();
        const QuizResult r = game.submitQuizAnswer(q.answer);
        CHECK(r.correct);
        CHECK(r.amount == Game::kQuizReward);
        CHECK(game.currentPlayer().money() == money + Game::kQuizReward);
        game.endTurn();
    }
    CHECK(static_cast<int>(seen.size()) == total); // 一轮内无重复

    // 答错扣钱
    const Question q = game.drawQuestionFor(jinjuLand);
    const int wrong = (q.answer + 1) % 4;
    const int money = game.currentPlayer().money();
    const QuizResult r = game.submitQuizAnswer(wrong);
    CHECK(!r.correct);
    CHECK(r.amount == -Game::kQuizPenalty);
    CHECK(game.currentPlayer().money() == money - Game::kQuizPenalty);

    // 同一道题不能重复提交
    const int after = game.currentPlayer().money();
    game.submitQuizAnswer(q.answer);
    CHECK(game.currentPlayer().money() == after);
}

// 11. 戏班开销与连片租金加成
void testTurnCostAndRentBonus()
{
    section("Turn cost (rising) and rent bonus for connected properties");
    Game game;
    game.setSeed(77u);
    game.start(2);

    // 回合开销随回合数递增：起始 30，每 10 回合 +5，上限 120
    CHECK(game.turnCost() == Game::kTurnCost);
    game.playerAt(0).setMoney(10000);
    game.playerAt(1).setMoney(10000);
    for (int i = 0; i < 10; ++i)
        game.endTurn();
    CHECK(game.turnCount() == 11);
    CHECK(game.turnCost() == Game::kTurnCost + Game::kTurnCostGrowth);
    CHECK(game.turnCost() <= Game::kTurnCostMax);

    // 连片加成：地主每多一块地，租金 +25%
    Game g2;
    g2.setSeed(9u);
    g2.start(2);
    const int baseRent1 = g2.lands()[1].rent();
    g2.playerAt(0).setPosition(1);
    CHECK(g2.buyCurrentLand());
    CHECK(g2.currentRent(1) == baseRent1); // 只有一块地，无加成

    g2.playerAt(0).setPosition(3);
    CHECK(g2.buyCurrentLand());
    g2.playerAt(0).setPosition(5);
    CHECK(g2.buyCurrentLand());
    CHECK(g2.playerAt(0).propertyCount() == 3);
    // 三块地：+50%
    CHECK(g2.currentRent(1) == baseRent1 * (100 + 2 * Game::kRentBonusPerProperty) / 100);

    // 交不出戏班开销 -> 破产
    Game g3;
    g3.setSeed(5u);
    g3.start(2);
    g3.playerAt(0).setMoney(Game::kTurnCost - 10);
    CHECK(g3.beginTurn());
    CHECK(g3.player(0).isBankrupt());
    CHECK(g3.player(0).money() == 0);
}

// 12. 选项位置分布：正确答案应均匀落在 A/B/C/D（不再总是 A）
void testAnswerPositionDistribution()
{
    section("Answer positions spread across A/B/C/D");
    Game game;
    game.setSeed(20261003u);
    game.start(2);

    int jinjuLand = -1;
    for (int i = 0; i < static_cast<int>(game.lands().size()); ++i) {
        if (game.lands()[i].isJinju()) {
            jinjuLand = i;
            break;
        }
    }
    CHECK(jinjuLand >= 0);

    int counts[4] = {0, 0, 0, 0};
    const int draws = 400;
    for (int i = 0; i < draws; ++i) {
        const Question q = game.drawQuestionFor(jinjuLand);
        CHECK(q.answer >= 0 && q.answer <= 3);
        counts[q.answer]++;
        game.submitQuizAnswer(q.answer); // 答完才能继续抽下一题
        game.endTurn();
    }
    for (int i = 0; i < 4; ++i)
        CHECK(counts[i] > draws / 10); // 每个位置至少出现 10%
    std::cout << "         A/B/C/D 分布 = " << counts[0] << " / " << counts[1] << " / "
              << counts[2] << " / " << counts[3] << "（共 " << draws << " 次抽题）\n";
}

// 13. 长时间随机对局：数值不变量
void testRandomGameInvariants()
{
    section("Random play: invariants over many turns");
    Game game;
    game.setSeed(2026u);
    game.start(4);

    const int maxTurns = 3000;
    int turns = 0;
    while (!game.isOver() && turns < maxTurns) {
        if (game.beginTurn()) { // 支付戏班开销
            if (!game.isOver())
                game.endTurn();
            ++turns;
            continue;
        }
        CHECK(!game.currentPlayer().isBankrupt());
        const int dice = game.rollDice();
        CHECK(dice >= 1 && dice <= 6);
        while (game.hasPendingSteps())
            game.moveOneStep();

        const LandEvent ev = game.settleLand();
        if (ev.canBuy && !game.isOver())
            game.buyCurrentLand();
        if (ev.needQuiz && !game.isOver()) {
            const Question q = game.drawQuestionFor(game.currentPlayer().position());
            game.submitQuizAnswer((q.answer + 1) % 4); // 故意答错，加快分出胜负
        }
        for (int i = 0; i < game.playerCount(); ++i) {
            CHECK(game.player(i).money() >= 0);
            CHECK(game.player(i).position() >= 0 && game.player(i).position() < kBoardSize);
        }
        if (!game.isOver())
            game.endTurn();
        ++turns;
    }
    std::cout << "         simulated turns: " << turns
              << ", alive players: " << game.aliveCount()
              << ", game over: " << (game.isOver() ? "yes" : "no") << "\n";
    CHECK(turns > 0);
    CHECK(game.aliveCount() >= 1);
}

} // namespace

int main()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "==== JinQuQiLv model tests ====\n";
    testPlayerBasics();
    testBoardData();
    testQuestionBank();
    testDice();
    testMoveAndSalary();
    testBuyAndRent();
    testBankruptcyAndVictory();
    testTurnSwitch();
    testQuizCategoryAssignment();
    testQuizDrawAndScore();
    testTurnCostAndRentBonus();
    testAnswerPositionDistribution();
    testRandomGameInvariants();

    std::cout << "==== " << (g_checks - g_failures) << "/" << g_checks << " checks passed";
    if (g_failures == 0)
        std::cout << " -- ALL TESTS PASSED ====\n";
    else
        std::cout << " -- " << g_failures << " FAILED ====\n";

#if defined(_WIN32)
    if (_isatty(_fileno(stdin))) {
        std::cout << "\n按回车键退出……";
        std::cin.get();
    }
#endif
    return g_failures == 0 ? 0 : 1;
}
