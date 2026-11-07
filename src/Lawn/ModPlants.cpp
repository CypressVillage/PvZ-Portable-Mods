// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModPlants.h"
#include "../LawnApp.h"
#include "graphics/GLImage.h"
#include "../SexyAppFramework/Common.h"
#include <cctype>
#include "../PvzpLib/Reanimator.h"
#include "Board.h"
#include "ModEntities.h"

namespace Mod
{
Plants::Entry::~Entry()
{
	if (animation) ReanimationFreeDefinition(animation.get());
}
Plants::~Plants() = default;

Plants::Plants(LawnApp& app, Runtime& runtime)
{
	mGameplayActive = !runtime.Plants().empty() || runtime.HasGameHooks();
	for (const auto& [id, content] : runtime.Plants())
	{
		auto entry = std::make_unique<Entry>();
		entry->id = id;
		const auto& data = content.definition;
		entry->name = data.at("name").get<std::string>();
		entry->description = data.at("description").get<std::string>();
		entry->health = data.at("health").get<int>();
		entry->card.reset(app.GetImage(Sexy::PathToU8(runtime.PlantAsset(id, true))));
		if (!entry->card || entry->card->GetWidth() != 80 || entry->card->GetHeight() != 80)
			throw Error(content.owner, "plant resources", id + ": card must be a readable 80x80 PNG");
		if (data.at("appearance").contains("reanim"))
		{
			auto path = runtime.PlantAsset(id);
			entry->animation = std::make_unique<ReanimatorDefinition>();
			entry->actions = data.at("appearance").at("animations");
			std::function<Sexy::Image*(const std::string&)> images = [&](const std::string& name) {
				auto& image = entry->animationImages[name];
				if (!image) image.reset(app.GetImage(Sexy::PathToU8(ResolveFile(path.parent_path(), name))));
				if (!image) throw Error(content.owner, "Reanim image", id + ": " + name);
				return image.get();
			};
			if (!ReanimationLoadDefinition(Sexy::PathToU8(path), entry->animation.get(), &images))
				throw Error(content.owner, "Reanim", id + ": could not load animation");
			Reanimation preview;
			preview.ReanimationInitialize(0, 0, entry->animation.get());
			for (const char* action : {"idle", "attack", "sleep"})
				if (!preview.TrackExists(entry->actions.at(action).get<std::string>().c_str()))
					throw Error(content.owner, "Reanim", id + ": missing action track: " + action);
			entry->image.reset(app.GetImage(Sexy::PathToU8(runtime.PlantAsset(id, true))));
		}
		else
		{
			entry->image.reset(app.GetImage(Sexy::PathToU8(runtime.PlantAsset(id))));
			if (!entry->image || entry->image->GetWidth() != 80 || entry->image->GetHeight() != 80)
				throw Error(content.owner, "plant resources", id + ": appearance must be a readable 80x80 PNG");
		}
		entry->upgradeFrom = data.value("upgrade", Data::object()).value("from", std::string());
		if (!entry->upgradeFrom.empty() && entry->upgradeFrom.find(':') == std::string::npos)
			entry->upgradeFrom = content.owner + ":" + entry->upgradeFrom;
		entry->fungus = data.value("traits", Data::object()).value("fungus", false);
		entry->nocturnal = data.value("traits", Data::object()).value("nocturnal", false);
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
		mEntries.push_back(std::move(entry));
	}
	for (const auto& entry : mEntries)
	{
		if (entry->upgradeFrom.empty()) continue;
		bool found = !BasePlantDefinition(entry->upgradeFrom).is_null();
		for (const auto& source : mEntries) found |= source->id == entry->upgradeFrom;
		if (!found || entry->id == entry->upgradeFrom)
			throw std::runtime_error(entry->id + ": invalid upgrade source: " + entry->upgradeFrom);
	}
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
	Data result = {{"type", id}, {"column", plant.mPlantCol}, {"row", plant.mRow}, {"health", plant.mPlantHealth}};
	if (plant.mIsOnBoard && plant.mBoard->mModEntities) result["entity"] = plant.mBoard->mModEntities->Handle(plant);
	return result;
}

Data BasePlantDefinition(const std::string& id)
{
	for (int i = 0; i < NUM_SEED_TYPES; ++i)
	{
		auto type = static_cast<SeedType>(i);
		if (PlantTypeId(type) != id) continue;
		const auto& def = GetPlantDefinition(type);
		Data result = {{"id", id}, {"name", def.mPlantName}, {"cost", def.mSeedCost}, {"cooldown", def.mRefreshTime}};
		// Special attacks are exposed only after their native parameters are shared.
		if (type == SEED_FUMESHROOM)
			result["attack"] = {{"damage", FUME_SHROOM_ATTACK.damage}, {"interval", def.mLaunchRate},
				{"interval_variation", SHOOTER_INTERVAL_VARIATION}, {"initial_min", 0}, {"initial_max", def.mLaunchRate},
				{"windup", FUME_SHROOM_ATTACK.shootingTicks - FUME_SHROOM_ATTACK.hitCounter},
				{"duration", FUME_SHROOM_ATTACK.shootingTicks}, {"damage_mode", "shield_and_body"}};
		return result;
	}
	return nullptr;
}

void PlayAnimation(Plant& plant, const std::string& action)
{
	const auto* entry = FindPlant(plant.mSeedType);
	if (!entry || !entry->animation) throw std::runtime_error("plant has no external animation");
	auto track = entry->actions.at(action).get<std::string>();
	plant.PlayBodyReanim(track.c_str(), action == "attack" ? REANIM_PLAY_ONCE_AND_HOLD : REANIM_LOOP, 0,
		entry->animation->mFPS);
}

void PlantHook(const char* event, Data& context)
{
	if (gLawnApp && gLawnApp->mModRuntime)
	{
		gLawnApp->mModRuntime->Dispatch(event, context);
		if (std::string_view(event) == "plant.removed") gLawnApp->mModRuntime->ForgetEntity(context.at("entity").get<std::string>());
	}
}
}
#endif
