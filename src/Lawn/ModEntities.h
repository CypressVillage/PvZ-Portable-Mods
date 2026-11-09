// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD
#include "../Mod/ModRuntime.h"
class Board;
class Plant;
class Zombie;
class Coin;

namespace Mod
{
class Entities
{
public:
	explicit Entities(Board& board);
	std::string Handle(const Plant& plant) const;
	std::string Handle(Zombie& zombie) const;
	std::string Handle(Coin& coin) const;
	Data Call(const std::string& operation, const Data& args);
private:
	Board& mBoard;
	std::string mPrefix;
	Plant* PlantFrom(const std::string& handle) const;
	Zombie* ZombieFrom(const std::string& handle) const;
	Coin* CoinFrom(const std::string& handle) const;
};
Data GameCall(const std::string& operation, const Data& arguments);
}
#endif
