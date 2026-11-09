// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModEntities.h"
#include "ModPlants.h"
#include "ModEffects.h"
#include "Board.h"
#include "Coin.h"
#include "../LawnApp.h"
#include "../GameConstants.h"

namespace Mod
{
Entities::Entities(Board& board) : mBoard(board)
{
	static uint64_t nextBoard = 0;
	mPrefix = std::to_string(++nextBoard) + ":";
}
std::string Entities::Handle(const Plant& plant) const
{
	return mPrefix + "p:" + std::to_string(mBoard.mPlants.DataArrayGetID(const_cast<Plant*>(&plant)));
}
std::string Entities::Handle(Zombie& zombie) const
{
	return mPrefix + "z:" + std::to_string(mBoard.mZombies.DataArrayGetID(&zombie));
}
std::string Entities::Handle(Coin& coin) const
{
	return mPrefix + "c:" + std::to_string(mBoard.mCoins.DataArrayGetID(&coin));
}
Plant* Entities::PlantFrom(const std::string& handle) const
{
	const auto prefix = mPrefix + "p:";
	if (!handle.starts_with(prefix)) return nullptr;
	auto* plant = mBoard.mPlants.DataArrayTryToGet(std::stoul(handle.substr(prefix.size())));
	return plant && !plant->mDead ? plant : nullptr;
}
Zombie* Entities::ZombieFrom(const std::string& handle) const
{
	const auto prefix = mPrefix + "z:";
	if (!handle.starts_with(prefix)) return nullptr;
	auto* zombie = mBoard.mZombies.DataArrayTryToGet(std::stoul(handle.substr(prefix.size())));
	return zombie && !zombie->mDead ? zombie : nullptr;
}
Coin* Entities::CoinFrom(const std::string& handle) const
{
	const auto prefix = mPrefix + "c:";
	if (!handle.starts_with(prefix)) return nullptr;
	auto* coin = mBoard.mCoins.DataArrayTryToGet(std::stoul(handle.substr(prefix.size())));
	return coin && !coin->mDead ? coin : nullptr;
}
Data Entities::Call(const std::string& operation, const Data& args)
{
	if (operation == "collectibles.collect")
	{
		auto* coin = CoinFrom(args.get<std::string>());
		if (!coin || coin->mIsBeingCollected) return false;
		coin->PlayCollectSound();
		coin->Collect();
		return true;
	}
	if (operation == "entities.get")
	{
		auto handle = args.get<std::string>();
		if (auto* p = PlantFrom(handle))
		{
			auto result = PlantContext(*p);
			result["kind"] = "plant";
			result["x"] = p->mX;
			result["y"] = p->mY;
			result["asleep"] = p->mIsAsleep;
			return result;
		}
		if (auto* z = ZombieFrom(handle))
			return {{"entity", handle}, {"kind", "zombie"}, {"row", z->mRow}, {"x", z->mPosX},
				{"y", z->mPosY}, {"has_metal", z->HasMagnetAttractableItem()}};
		return nullptr;
	}
	if (operation == "zombies.query")
	{
		auto* source = PlantFrom(args.at("source").get<std::string>());
		if (!source) throw std::runtime_error("source plant is no longer alive");
		if (args.value("direction", "forward") != "forward" || args.value("target", "ground") != "ground")
			throw std::runtime_error("query currently supports forward ground attacks only");
		int row = args.value("row", source->mRow);
		if (row < 0 || row >= MAX_GRID_SIZE_Y) throw std::runtime_error("row is outside the lawn");
		Data result = Data::array();
		Rect attack(source->mX + 60, mBoard.GridToPixelY(source->mPlantCol, row), BOARD_WIDTH - source->mX - 60, source->mHeight);
		for (auto* zombie : mBoard.mZombies)
		{
			if (zombie->mDead || zombie->IsDeadOrDying() || zombie->mMindControlled) continue;
			if ((zombie->mRow != row && zombie->mZombieType != ZOMBIE_BOSS) ||
				zombie->mOnHighGround != source->IsOnHighGround() || !zombie->EffectedByDamage(9)) continue;
			auto rect = zombie->GetZombieRect();
			if (GetRectOverlap(attack, rect) > 0) result.push_back(Handle(*zombie));
		}
		return result;
	}
	if (operation == "zombies.damage")
	{
		auto* zombie = ZombieFrom(args.at("entity").get<std::string>());
		const auto& options = args.at("options");
		if (!PlantFrom(options.at("source").get<std::string>())) throw std::runtime_error("source plant is no longer alive");
		const auto& amount = options.at("amount");
		if (!amount.is_number_integer() || amount < 0 || amount > INT32_MAX) throw std::runtime_error("damage must be a nonnegative 32-bit integer");
		int damage = amount.get<int>();
		auto mode = options.value("mode", "normal");
		unsigned int flags = 0;
		if (mode == "shield_and_body") flags = 1U << DAMAGE_HITS_SHIELD_AND_BODY;
		else if (mode != "normal") throw std::runtime_error("unsupported damage mode");
		if (!zombie || zombie->IsDeadOrDying()) return false;
		zombie->TakeDamage(damage, flags);
		return true;
	}
	if (operation == "plants.play_animation")
	{
		auto* plant = PlantFrom(args.at("entity").get<std::string>());
		if (!plant) return false;
		PlayAnimation(*plant, args.at("action").get<std::string>());
		return true;
	}
	if (operation == "effects.lightning")
	{
		auto* plant = PlantFrom(args.at("source").get<std::string>());
		if (!plant) return false;
		mBoard.mModEffects->Lightning(plant->mX + 70, plant->mY + 32, BOARD_WIDTH, plant->mRow);
		return true;
	}
	throw std::runtime_error("unsupported game operation: " + operation);
}
Data GameCall(const std::string& operation, const Data& arguments)
{
	if (operation == "plants.get") return BasePlantDefinition(arguments.get<std::string>());
	return gLawnApp->mBoard->mModEntities->Call(operation, arguments);
}
}
#endif
