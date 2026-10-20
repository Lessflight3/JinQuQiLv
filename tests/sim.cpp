// 对局时长模拟：用不同答题水平跑大量对局，统计"多少回合能分出胜负"，
// 用于把经济参数调到一局 10~15 分钟的量级。
// 编译：g++ -std=c++17 -O2 tests/sim.cpp model/*.cpp -o sim

#include "../model/game.h"
#include "../model/questionbank.h"

#include <algorithm>
#include <iostream>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

enum Policy
{
    AllWrong = 0, // 每题都答错
    HalfRight = 1, // 一半答对
    AllRight = 2  // 每题都答对（最耗时的情况）
};

struct Result
{
    int turns = 0;
    bool over = false;
};

Result runGame(int players, Policy policy, unsigned seed)
{
    Game game;
    game.setSeed(seed);
    game.start(players);

    Result r;
    const int cap = 5000;
    while (!game.isOver() && r.turns < cap) {
        if (game.beginTurn()) { // 先付戏班开销；交不出即出局
            if (!game.isOver())
                game.endTurn();
            ++r.turns;
            continue;
        }
        game.rollDice();
        while (game.hasPendingSteps())
            game.moveOneStep();

        const LandEvent ev = game.settleLand();
        if (ev.canBuy && !game.isOver())
            game.buyCurrentLand();
        if (ev.needQuiz && !game.isOver()) {
            const Question q = game.drawQuestionFor(game.currentPlayer().position());
            int choice = q.answer; // 默认答对
            if (policy == AllWrong)
                choice = (q.answer + 1) % 4;
            else if (policy == HalfRight && (r.turns % 2) == 1)
                choice = (q.answer + 1) % 4;
            game.submitQuizAnswer(choice);
        }
        if (!game.isOver())
            game.endTurn();
        ++r.turns;
    }
    r.over = game.isOver();
    return r;
}

void report(const char *name, int players, Policy policy)
{
    std::vector<int> turns;
    int unfinished = 0;
    const int games = 200;
    for (int i = 0; i < games; ++i) {
        const Result r = runGame(players, policy, 1000u + i * 7919u);
        if (!r.over)
            ++unfinished;
        turns.push_back(r.turns);
    }
    std::sort(turns.begin(), turns.end());
    const double avg = [&] {
        double s = 0;
        for (int t : turns)
            s += t;
        return s / turns.size();
    }();
    std::cout << name << "  人数=" << players << "  每题命中率="
              << (policy == AllWrong ? 0 : (policy == HalfRight ? 50 : 100)) << "%"
              << "  结束率=" << (100 - unfinished * 100 / games) << "%"
              << "  平均结束回合(总出手次数)=" << int(avg)
              << "  中位数=" << turns[turns.size() / 2]
              << "  最快=" << turns.front() << "  最慢=" << turns.back() << "\n";
}

} // namespace

int main()
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "==== 对局时长模拟（各 200 局）====\n";
    std::cout << "经济参数：初始 " << Game::kInitialMoney << " 元，过起点 +"
              << Game::kStartSalary << "，答对 +" << Game::kQuizReward << "，答错 -"
              << Game::kQuizPenalty << "，租金约为地价六成\n\n";

    for (int players : {2, 3, 4}) {
        report("全答对", players, AllRight);
        report("半对半错", players, HalfRight);
        report("全答错", players, AllWrong);
        std::cout << "\n";
    }
    return 0;
}
