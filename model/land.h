#ifndef LAND_H
#define LAND_H

#include <string>

// 地块类型：起点 / 普通地块 / 晋剧文化地块
enum class LandType
{
    Start,
    Normal,
    Jinju
};

// 地块类：名称、类型、购买价格、租金、拥有者
class Land
{
public:
    Land() = default;
    Land(const std::string &name, LandType type, int price = 0, int rent = 0,
         const std::string &key = std::string());

    const std::string &name() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    // 素材键：用于匹配地域风貌插画 / 真实照片文件名
    const std::string &key() const { return m_key; }
    void setKey(const std::string &key) { m_key = key; }

    LandType type() const { return m_type; }
    void setType(LandType type) { m_type = type; }
    bool isStart() const { return m_type == LandType::Start; }
    bool isNormal() const { return m_type == LandType::Normal; }
    bool isJinju() const { return m_type == LandType::Jinju; }

    int price() const { return m_price; }
    void setPrice(int price) { m_price = price; }
    int rent() const { return m_rent; }
    void setRent(int rent) { m_rent = rent; }

    // 拥有者下标，-1 表示无人拥有
    int owner() const { return m_owner; }
    void setOwner(int owner) { m_owner = owner; }
    bool isOwned() const { return m_owner >= 0; }
    void clearOwner() { m_owner = -1; }

private:
    std::string m_name;
    std::string m_key;
    LandType m_type = LandType::Normal;
    int m_price = 0;
    int m_rent = 0;
    int m_owner = -1;
};

#endif // LAND_H
