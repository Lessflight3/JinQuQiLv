#include "land.h"

Land::Land(const std::string &name, LandType type, int price, int rent, const std::string &key)
    : m_name(name), m_key(key), m_type(type), m_price(price), m_rent(rent)
{
}
