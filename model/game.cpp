#include "game.h"

#include "questionbank.h"

#include <algorithm>
#include <random>

namespace {

const char *kDefaultNames[4] = {"玩家一", "玩家二", "玩家三", "玩家四"};

} // namespace

std::vector<Land> createBoard()
{
    // 24 格按顺时针排布：0 号起点；15 个普通地块（山西文化地标）；8 个晋剧文化地块
    // （四大类题目各占 2 块）。
    std::vector<Land> board;
    board.reserve(kBoardSize);
    board.emplace_back("起点", LandType::Start, 0, 0);
    board.emplace_back("晋祠", LandType::Normal, 200, 120, "jinci");
    board.emplace_back("脸谱阁", LandType::Jinju, 0, 0);
    board.emplace_back("乔家大院", LandType::Normal, 180, 110, "qiaojiadayuan");
    board.emplace_back("水袖台", LandType::Jinju, 0, 0);
    board.emplace_back("平遥古城", LandType::Normal, 160, 95, "pingyaogucheng");
    board.emplace_back("云冈石窟", LandType::Normal, 220, 130, "yungangshiku");
    board.emplace_back("梆子腔", LandType::Jinju, 0, 0);
    board.emplace_back("壶口瀑布", LandType::Normal, 240, 145, "hukoupubu");
    board.emplace_back("琴师坊", LandType::Jinju, 0, 0);
    board.emplace_back("五台山", LandType::Normal, 200, 120, "wutaishan");
    board.emplace_back("悬空寺", LandType::Normal, 180, 110, "xuankongsi");
    board.emplace_back("髯口坊", LandType::Jinju, 0, 0);
    board.emplace_back("应县木塔", LandType::Normal, 160, 95, "yingxianmuta");
    board.emplace_back("雁门关", LandType::Normal, 260, 155, "yanmenguan");
    board.emplace_back("戏服阁", LandType::Jinju, 0, 0);
    board.emplace_back("娘子关", LandType::Normal, 200, 120, "niangziguan");
    board.emplace_back("绵山", LandType::Normal, 220, 130, "mianshan");
    board.emplace_back("翎子台", LandType::Jinju, 0, 0);
    board.emplace_back("碛口古镇", LandType::Normal, 140, 85, "qikouguzhen");
    board.emplace_back("梨园堂", LandType::Jinju, 0, 0);
    board.emplace_back("老陈醋坊", LandType::Normal, 140, 85, "laochencufang");
    board.emplace_back("八泉峡", LandType::Normal, 180, 110, "baquanxia");
    board.emplace_back("洪洞大槐树", LandType::Normal, 120, 70, "hongtongdahuaishu");
    return board;
}

Game::Game()
    : m_rng(std::random_device{}())
{
}

void Game::setSeed(unsigned int seed)
{
    m_rng.seed(seed);
}

void Game::start(int playerCount, const std::vector<std::string> &names)
{
    if (playerCount < 2)
        playerCount = 2;
    if (playerCount > 4)
        playerCount = 4;

    m_lands = createBoard();
    assignQuizCategories(); // 把四类题目随机散布到 9 个晋剧地块
    m_players.clear();
    for (int i = 0; i < playerCount; ++i) {
        std::string name = kDefaultNames[i];
        if (i < static_cast<int>(names.size()) && !names[i].empty())
            name = names[i];
        m_players.emplace_back(name, kInitialMoney);
    }

    m_current = 0;
    m_lastDice = 0;
    m_pendingSteps = 0;
    m_turn = 1;
    m_winner = -1;
    m_over = false;
    m_quizOpen = false;
    m_lastQuestion = Question();
    for (int i = 0; i < kQuizCategoryCount; ++i)
        refillBag(static_cast<QuizCategory>(i));
}

void Game::reset()
{
    m_players.clear();
    m_lands = createBoard();
    m_landCategories.clear();
    for (std::vector<Question> &bag : m_bags)
        bag.clear();
    m_current = 0;
    m_lastDice = 0;
    m_pendingSteps = 0;
    m_turn = 1;
    m_winner = -1;
    m_over = false;
    m_quizOpen = false;
}

int Game::aliveCount() const
{
    int count = 0;
    for (const Player &p : m_players) {
        if (!p.isBankrupt())
            ++count;
    }
    return count;
}

int Game::rollDice()
{
    if (m_over || m_players.empty())
        return 0;
    std::uniform_int_distribution<int> dist(1, 6);
    m_lastDice = dist(m_rng);
    m_pendingSteps = m_lastDice;
    return m_lastDice;
}

bool Game::beginTurn()
{
    if (m_over || m_players.empty())
        return false;
    Player &p = m_players[m_current];
    if (p.isBankrupt())
        return false;

    // 戏班开销：场地、行头、人工的固定支出；随回合数缓慢上涨，
    // 既保证前期能安心买地，又保证后期对局一定会收敛。
    const int cost = turnCost();
    const int paid = std::min(cost, p.money());
    p.addMoney(-paid);
    if (paid < cost) {
        bankruptPlayer(m_current);
        checkGameOver();
        return true;
    }
    return false;
}

int Game::turnCost() const
{
    const int extra = (m_turn / 10) * kTurnCostGrowth;
    return std::min(kTurnCostMax, kTurnCost + extra);
}

bool Game::moveOneStep()
{
    if (m_over || m_pendingSteps <= 0 || m_players.empty())
        return false;

    Player &p = m_players[m_current];
    const int next = (p.position() + 1) % kBoardSize;
    const bool passedStart = (next == 0); // 绕棋盘一圈，回到/越过起点
    p.setPosition(next);
    --m_pendingSteps;
    if (passedStart)
        p.addMoney(kStartSalary); // 经过起点领取补贴
    return passedStart;
}

int Game::moveCurrent(int steps)
{
    if (m_over || m_players.empty() || steps <= 0)
        return 0;

    m_pendingSteps = steps;
    int laps = 0;
    while (m_pendingSteps > 0) {
        if (moveOneStep())
            ++laps;
    }
    return laps;
}

LandEvent Game::settleLand()
{
    LandEvent ev;
    if (m_over || m_players.empty() || m_lands.empty())
        return ev;

    Player &p = m_players[m_current];
    const int index = p.position();
    Land &land = m_lands[index];

    ev.player = m_current;
    ev.land = index;
    ev.owner = land.owner();

    if (land.isStart()) {
        ev.type = EventType::StartReward;
        ev.text = "停在起点休整，本回合无额外事件。";
        return ev;
    }

    if (land.isJinju()) {
        ev.type = EventType::JinjuQuiz;
        ev.needQuiz = true;
        ev.text = "踩到晋剧文化地块【" + land.name() + "】，抽取一道【"
                  + quizCategoryName(quizCategoryOf(index)) + "】题目。";
        return ev;
    }

    // 普通地块：无人拥有 -> 可购买
    if (!land.isOwned()) {
        ev.type = EventType::UnownedLand;
        ev.canBuy = (p.money() >= land.price());
        ev.text = "【" + land.name() + "】是无人拥有的空地，售价 " + std::to_string(land.price())
                  + " 元" + (ev.canBuy ? "，可以购买。" : "，现金不足，无法购买。");
        return ev;
    }

    // 自己的地
    if (land.owner() == m_current) {
        ev.type = EventType::OwnLand;
        ev.text = "回到自己的地产【" + land.name() + "】，安心收租。";
        return ev;
    }

    // 其他玩家的地：支付租金
    const int owner = land.owner();
    const int rent = currentRent(index);
    const int paid = std::min(rent, p.money());
    p.addMoney(-paid);
    m_players[owner].addMoney(paid);
    ev.type = EventType::PayRent;
    ev.amount = -paid;
    ev.text = "【" + p.name() + "】踩到【" + m_players[owner].name() + "】的【" + land.name()
              + "】，支付租金 " + std::to_string(paid) + " 元。";

    if (paid < rent) {
        ev.type = EventType::Bankrupt;
        ev.playerBankrupt = true;
        ev.text += " 现金不足，宣告破产出局！";
        bankruptPlayer(m_current);
    }
    return ev;
}

bool Game::canBuyCurrentLand() const
{
    if (m_over || m_players.empty() || m_lands.empty())
        return false;
    const Player &p = m_players[m_current];
    const Land &land = m_lands[p.position()];
    return land.isNormal() && !land.isOwned() && p.money() >= land.price() && !p.isBankrupt();
}

int Game::currentRent(int landIndex) const
{
    if (landIndex < 0 || landIndex >= static_cast<int>(m_lands.size()))
        return 0;
    const Land &land = m_lands[landIndex];
    const int owner = land.owner();
    if (owner < 0 || owner >= static_cast<int>(m_players.size()))
        return land.rent();
    const int owned = m_players[owner].propertyCount();
    const int bonus = std::max(0, owned - 1) * kRentBonusPerProperty;
    return land.rent() * (100 + bonus) / 100;
}

bool Game::buyCurrentLand()
{
    if (isOver() || m_players.empty() || m_lands.empty())
        return false;
    Player &p = m_players[m_current];
    Land &land = m_lands[p.position()];
    if (!land.isNormal() || land.isOwned() || p.money() < land.price() || p.isBankrupt())
        return false;

    p.addMoney(-land.price());
    land.setOwner(m_current);
    p.addProperty(p.position());
    return true;
}

void Game::assignQuizCategories()
{
    // 每轮把四大类随机排列后依次分配，共分 kJinjuPerCategory 轮，
    // 因此每个类别恰好占 kJinjuPerCategory（=2）个晋剧地块，位置每局不同
    m_landCategories.assign(m_lands.size(), QuizCategory::CommonSense);

    std::vector<int> jinjuIndices;
    for (int i = 0; i < static_cast<int>(m_lands.size()); ++i) {
        if (m_lands[i].isJinju())
            jinjuIndices.push_back(i);
    }

    std::vector<QuizCategory> order;
    while (order.size() < jinjuIndices.size()) {
        std::vector<QuizCategory> round = {QuizCategory::CommonSense, QuizCategory::Tune,
                                           QuizCategory::Aria, QuizCategory::Picture};
        std::shuffle(round.begin(), round.end(), m_rng);
        order.insert(order.end(), round.begin(), round.end());
    }
    order.resize(jinjuIndices.size());

    for (size_t i = 0; i < jinjuIndices.size(); ++i)
        m_landCategories[jinjuIndices[i]] = order[i];
}

QuizCategory Game::quizCategoryOf(int landIndex) const
{
    if (landIndex < 0 || landIndex >= static_cast<int>(m_landCategories.size()))
        return QuizCategory::CommonSense;
    return m_landCategories[landIndex];
}

QuizCategory Game::currentQuizCategory() const
{
    if (m_players.empty())
        return QuizCategory::CommonSense;
    return quizCategoryOf(m_players[m_current].position());
}

void Game::refillBag(QuizCategory category)
{
    std::vector<Question> &bag = m_bags[static_cast<int>(category)];
    bag = questionsOf(category);
    std::shuffle(bag.begin(), bag.end(), m_rng);
}

Question Game::drawQuestionFor(int landIndex)
{
    const QuizCategory category = quizCategoryOf(landIndex);
    std::vector<Question> &bag = m_bags[static_cast<int>(category)];
    if (bag.empty())
        refillBag(category); // 该类题目抽完一轮后重新洗牌，避免短期重复
    if (bag.empty()) {
        m_quizOpen = false;
        return Question();
    }

    // 打乱选项顺序：保证正确答案在 A/B/C/D 四个位置均匀出现，
    // 而不是取决于题库编写时的顺序。
    const Question original = bag.back();
    bag.pop_back();

    std::array<int, 4> order = {0, 1, 2, 3};
    std::shuffle(order.begin(), order.end(), m_rng);

    Question shuffled = original;
    for (int i = 0; i < 4; ++i) {
        shuffled.options[static_cast<size_t>(i)] = original.options[static_cast<size_t>(order[i])];
        if (order[i] == original.answer)
            shuffled.answer = i;
    }
    m_lastQuestion = shuffled;
    m_quizOpen = true;
    return m_lastQuestion;
}

QuizResult Game::submitQuizAnswer(int choice)
{
    QuizResult result;
    if (m_over || !m_quizOpen || m_players.empty())
        return result;

    m_quizOpen = false;
    Player &p = m_players[m_current];

    if (choice == m_lastQuestion.answer) {
        result.correct = true;
        result.amount = kQuizReward;
        p.addMoney(kQuizReward);
        result.text = "回答正确！奖励 " + std::to_string(kQuizReward) + " 元。";
        return result;
    }

    const int paid = std::min(kQuizPenalty, p.money());
    p.addMoney(-kQuizPenalty);
    result.correct = false;
    result.amount = -paid;
    result.text = "回答错误，扣除 " + std::to_string(paid) + " 元。";
    if (paid < kQuizPenalty) {
        result.bankrupt = true;
        result.text += " 现金不足，宣告破产出局！";
        bankruptPlayer(m_current);
    }
    return result;
}

void Game::bankruptPlayer(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= static_cast<int>(m_players.size()))
        return;

    Player &p = m_players[playerIndex];
    if (p.money() < 0)
        p.setMoney(0);
    p.setBankrupt(true);
    // 破产玩家的地产重新回到可购买状态
    for (int landIndex : p.properties()) {
        if (landIndex >= 0 && landIndex < static_cast<int>(m_lands.size()))
            m_lands[landIndex].clearOwner();
    }
    p.clearProperties();
}

void Game::forceBankrupt(int playerIndex)
{
    bankruptPlayer(playerIndex);
    checkGameOver();
}

void Game::checkGameOver()
{
    int alive = -1;
    int count = 0;
    for (int i = 0; i < static_cast<int>(m_players.size()); ++i) {
        if (!m_players[i].isBankrupt()) {
            ++count;
            alive = i;
        }
    }
    if (count <= 1) {
        m_over = true;
        m_winner = (count == 1 ? alive : -1);
    }
}

void Game::endTurn()
{
    if (m_over || m_players.empty())
        return;

    checkGameOver();
    if (m_over)
        return;

    const int size = static_cast<int>(m_players.size());
    for (int step = 1; step <= size; ++step) {
        const int candidate = (m_current + step) % size;
        if (!m_players[candidate].isBankrupt()) {
            m_current = candidate;
            break;
        }
    }
    ++m_turn;
    m_pendingSteps = 0;
}
