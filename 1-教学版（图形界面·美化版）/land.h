#ifndef LAND_H
#define LAND_H

#include <QString>

// ============================================================
//  地块类 Land
//  知识点：类的定义、枚举、构造函数
// ============================================================

// 地块的三种类型：起点 / 普通地块 / 答题地块
enum LandType
{
    TYPE_START = 0,   // 起点
    TYPE_NORMAL = 1,  // 普通地块（可以买、收租）
    TYPE_QUIZ = 2     // 晋剧答题地块
};

class Land
{
public:
    // 构造函数：参数分别是 名字、类型、价格、租金
    Land(QString name, LandType type, int price, int rent)
    {
        m_name = name;
        m_type = type;
        m_price = price;
        m_rent = rent;
        m_owner = -1;     // -1 表示还没有人买，0/1 表示玩家编号
    }

    // 默认构造函数（数组里会自动调用这个）
    Land()
    {
        m_type = TYPE_NORMAL;
        m_price = 0;
        m_rent = 0;
        m_owner = -1;
    }

    QString name() const { return m_name; }
    LandType type() const { return m_type; }
    int price() const { return m_price; }
    int rent() const { return m_rent; }
    int owner() const { return m_owner; }          // 拥有者编号，-1 表示没人买
    bool hasOwner() const { return m_owner >= 0; } // 有没有被买走

    void setOwner(int playerIndex) { m_owner = playerIndex; }

private:
    QString m_name;   // 名字
    LandType m_type;  // 类型
    int m_price;      // 价格
    int m_rent;       // 租金
    int m_owner;      // 拥有者：-1 没人买，0 玩家1，1 玩家2
};

#endif // LAND_H
