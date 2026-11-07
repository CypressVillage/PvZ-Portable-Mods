// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD

#include "Plant.h"
#include "../Mod/ModRuntime.h"
#include <memory>
#include <vector>

class LawnApp;
namespace Sexy { class Image; }

namespace Mod
{
// Above every vanilla seed/pseudo-seed. These numbers are session-local only.
inline constexpr int FirstPlantType = 128;
static_assert(FirstPlantType > SEED_ZOMBIE_IMP);

class Plants
{
public:
	struct Entry
	{
		std::string id;
		std::string name;
		std::string description;
		int health;
		std::unique_ptr<Sexy::Image> image;
		std::unique_ptr<Sexy::Image> card;
		Sexy::Image* imagePointer = nullptr;
		PlantDefinition definition{};
		~Entry();
	};
	Plants(LawnApp& app, Runtime& runtime);
	~Plants();
	const Entry* Find(SeedType type) const;
	int EndType() const { return FirstPlantType + static_cast<int>(mEntries.size()); }
	bool Empty() const { return mEntries.empty(); }
	bool GameplayActive() const { return mGameplayActive; }
private:
	std::vector<std::unique_ptr<Entry>> mEntries;
	bool mGameplayActive = false;
};

const Plants::Entry* FindPlant(SeedType type);
// Vanilla IDs use base:<lowercase PlantDefinition::mPlantName>.
std::string PlantTypeId(SeedType type);
Data PlantContext(const Plant& plant);
void PlantHook(const char* event, Data& context);
bool HasPlayablePlants();
bool GameplayActive();
}
#endif
