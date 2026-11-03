// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ModPlants.h"
#include "../LawnApp.h"
#include "graphics/GLImage.h"
#include "../SexyAppFramework/Common.h"
#include <cctype>
#include <iomanip>
#include <sstream>

namespace Mod
{
Plants::Entry::~Entry() = default;
Plants::~Plants() = default;

Plants::Plants(LawnApp& app, Runtime& runtime)
{
	uint64_t hash = 14695981039346656037ULL;
	auto fingerprint = [&](const std::string& value) {
		for (unsigned char ch : value) { hash ^= ch; hash *= 1099511628211ULL; }
		hash ^= 0xff; hash *= 1099511628211ULL;
	};
	fingerprint(runtime.ConfigurationIdentity());
	mGameplayActive = !runtime.Plants().empty() || runtime.HasGameHooks();
	for (const auto& [id, content] : runtime.Plants())
	{
		auto entry = std::make_unique<Entry>();
		entry->id = id;
		const auto& data = content.definition;
		entry->name = data.at("name").get<std::string>();
		entry->description = data.at("description").get<std::string>();
		entry->health = data.at("health").get<int>();
		entry->image.reset(app.GetImage(Sexy::PathToU8(runtime.PlantAsset(id))));
		entry->card.reset(app.GetImage(Sexy::PathToU8(runtime.PlantAsset(id, true))));
		for (auto* image : {entry->image.get(), entry->card.get()})
			if (!image || image->GetWidth() != 80 || image->GetHeight() != 80)
				throw Error(content.owner, "plant resources", id + ": appearance and card images must be readable 80x80 PNGs");
		entry->imagePointer = entry->image.get();
		entry->definition = {
			.mSeedType = static_cast<SeedType>(EndType()),
			.mPlantImage = &entry->imagePointer,
			.mReanimationType = REANIM_NONE,
			.mPacketIndex = 0,
			.mSeedCost = data.at("cost").get<int>(),
			.mRefreshTime = data.at("cooldown").get<int>(),
			.mSubClass = SUBCLASS_NORMAL,
			.mLaunchRate = 0,
			.mPlantName = entry->name.c_str(),
		};
		fingerprint(id);
		fingerprint(data.dump());
		mEntries.push_back(std::move(entry));
	}
	std::ostringstream key;
	key << std::hex << std::setw(16) << std::setfill('0') << hash;
	mProfileKey = key.str();
}

const Plants::Entry* Plants::Find(SeedType type) const
{
	int index = static_cast<int>(type) - FirstPlantType;
	return index >= 0 && index < static_cast<int>(mEntries.size()) ? mEntries[index].get() : nullptr;
}

const Plants::Entry* FindPlant(SeedType type)
{
	return gLawnApp && gLawnApp->mModPlants ? gLawnApp->mModPlants->Find(type) : nullptr;
}

bool HasPlayablePlants()
{
	return gLawnApp && gLawnApp->mModPlants && !gLawnApp->mModPlants->Empty();
}

bool GameplayActive()
{
	return gLawnApp && gLawnApp->mModPlants && gLawnApp->mModPlants->GameplayActive();
}

std::string PlantTypeId(SeedType type)
{
	std::string id;
	if (const auto* definition = FindPlant(type)) id = definition->id;
	else if (type >= 0 && type < NUM_SEED_TYPES)
	{
		id = gPlantDefs[type].mPlantName;
		for (char& ch : id) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		id = "base:" + id;
	}
	return id;
}

Data PlantContext(const Plant& plant)
{
	auto id = PlantTypeId(plant.mSeedType);
	if (id.empty()) return Data::object();
	return {{"type", id}, {"column", plant.mPlantCol}, {"row", plant.mRow}, {"health", plant.mPlantHealth}};
}

void PlantHook(const char* event, Data& context)
{
	if (gLawnApp && gLawnApp->mModRuntime) gLawnApp->mModRuntime->Dispatch(event, context);
}
}
