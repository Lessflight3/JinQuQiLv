#ifndef PLAYER_H
#define PLAYER_H

#include <QString>

// ============================================================
//  玩家类 Player
//  知识点：类的定义、成员变量、构造函数、成员函数
// ============================================================
class Player
{
public:
    // 默认构造函数：写成 Player players[2]; 这种数组时需要它
    Player()
    {
        m_name = "玩家";
        m_money = 0;
        m_pos = 0;
        m_out = false;
        m_landCount = 0;
    }

    // 构造函数：创建玩家对象时自动调用
    // 参数 name 是名字，startMoney 是初始金钱
    Player(QString name, int startMoney)
    {
        m_name = name;        // 把参数保存到成员变量里
        m_money = startMoney;
        m_pos = 0;            // 位置从 0 号格（起点）开始
        m_out = false;        // 一开始没有出局
        m_landCount = 0;      // 一开始没有地块
    }

    // ---- 以下都是成员函数，用来读取/修改成员变量 ----

    QString name() const { return m_name; }     // 名字
    int money() const { return m_money; }       // 当前金钱
    int pos() const { return m_pos; }           // 当前位置（0~15）
    bool isOut() const { return m_out; }        // 是否已破产
    int landCount() const { return m_landCount; } // 拥有几块地

    void addMoney(int delta) { m_money += delta; }  // 加钱（负数就是扣钱）
    void setPos(int p) { m_pos = p; }               // 移动位置
    void setOut(bool out) { m_out = out; }           // 设置破产状态
    void addLand() { m_landCount = m_landCount + 1; } // 买地后 +1

private:
    // ---- 成员变量（每个玩家对象都有自己的一份）----
    QString m_name;   // 名字
    int m_money;      // 金钱
    int m_pos;        // 位置
    bool m_out;       // 是否出局
    int m_landCount;  // 拥有地块数量
};

#endif // PLAYER_H
