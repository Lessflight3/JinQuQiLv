#include "player.h"

#include <algorithm>

Player::Player(const std::string &name, int money)
    : m_name(name), m_money(money)
{
}

void Player::addProperty(int landIndex)
{
    if (!owns(landIndex))
        m_properties.push_back(landIndex);
}

bool Player::owns(int landIndex) const
{
    return std::find(m_properties.begin(), m_properties.end(), landIndex) != m_properties.end();
}
