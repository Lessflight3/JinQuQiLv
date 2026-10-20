#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>

// 玩家类：姓名、金钱、当前位置、持有地块、是否破产
class Player
{
public:
    Player() = default;
    Player(const std::string &name, int money = 2000);

    const std::string &name() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    int money() const { return m_money; }
    void setMoney(int money) { m_money = money; }
    void addMoney(int delta) { m_money += delta; }

    int position() const { return m_position; }
    void setPosition(int position) { m_position = position; }

    const std::vector<int> &properties() const { return m_properties; }
    void addProperty(int landIndex);
    void clearProperties() { m_properties.clear(); }
    bool owns(int landIndex) const;
    int propertyCount() const { return static_cast<int>(m_properties.size()); }

    bool isBankrupt() const { return m_bankrupt; }
    void setBankrupt(bool bankrupt) { m_bankrupt = bankrupt; }

private:
    std::string m_name;            // 玩家昵称
    int m_money = 2000;            // 现金
    int m_position = 0;            // 当前所在棋盘格下标
    std::vector<int> m_properties; // 持有地块下标
    bool m_bankrupt = false;       // 是否已破产出局
};

#endif // PLAYER_H
