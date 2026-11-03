// SPDX-License-Identifier: LGPL-3.0-or-later
// Alternate test entry point, linked with the game's objects instead of main.cpp.
// Run with an isolated -savedir and a resource tree containing plain-plant.
#include "LawnApp.h"
#include "Resources.h"
#include "PvzpLib/PvzpStringFile.h"
#include "Lawn/Board.h"
#include "Lawn/ModPlants.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/CursorObject.h"
#include "Lawn/Zombie.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/System/SaveGame.h"
#include "Lawn/Widget/SeedChooserScreen.h"
#include "graphics/GLImage.h"
#include "graphics/Graphics.h"
#include <iostream>

static void Check(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
	using namespace Sexy;
	try
	{
		PvzpStringListSetColors(gLawnStringFormats, gLawnStringFormatCount);
		gExtractResourcesByName = ExtractResourcesByName;
		gLawnApp = new LawnApp();
		gLawnApp->SetArgs(argc, argv);
		gLawnApp->Init();
		Check(!gLawnApp->mLoadingFailed && !gLawnApp->mShutdown, "initialization failed");
		const bool modded = Mod::HasPlayablePlants();
		Check((GetAppDataPath("userdata").find("mod-profiles/") != std::string::npos) == modded, "incorrect profile isolation");
		// Load synchronously: no background thread ever calls Lua.
		gLawnApp->LoadingThreadProc();
		gLawnApp->mLoadingThreadCompleted = true;
		Check(!gLawnApp->mLoadingFailed, "resource loading failed");
		gLawnApp->mPlayerInfo = gLawnApp->mProfileMgr->AddProfile("Mod API test");
		gLawnApp->mPlayerInfo->mLevel = 7;
		gLawnApp->mGameMode = GAMEMODE_ADVENTURE;
		gLawnApp->MakeNewBoard();
		auto* board = gLawnApp->mBoard;
		board->InitLevel();
		gLawnApp->mGameScene = SCENE_PLAYING;
		if (!modded)
		{
			Check(!Mod::GameplayActive(), "disabled mod hooks remained active");
			Check(Plant::GetCost(SEED_PEASHOOTER) == 100, "vanilla cost changed");
			auto* vanilla = board->AddPlant(2, 2, SEED_PEASHOOTER);
			Check(vanilla->mPlantHealth == 300 && vanilla->mSubclass == SUBCLASS_SHOOTER, "vanilla initialization changed");
			Check(board->NeedSaveGame(), "vanilla saving disabled");
			auto path = GetAppDataPath("vanilla.v4");
			Check(LawnSaveGame(board, path), "vanilla save failed");
			// Match LawnApp::TryLoadGame: restore into a fresh Board, not over
			// live particle/reanimation holders belonging to an existing level.
			gLawnApp->KillBoard();
			gLawnApp->MakeNewBoard();
			board = gLawnApp->mBoard;
			Check(LawnLoadGame(board, path), "vanilla restore failed");
			std::cout << "vanilla regression passed (identity, initialization, save, restore)\n";
			gLawnApp->Shutdown();
			delete gLawnApp;
			gLawnApp = nullptr;
			return 0;
		}
		const auto type = static_cast<SeedType>(Mod::FirstPlantType);
		Check(Mod::FindPlant(type)->id == "plain-plant:stump", "wrong plant identity");
		Check(Plant::GetCost(type) == 50 && Plant::GetRefreshTime(type) == 750, "definition queries failed");
		Check(Plant::GetNameString(type) == "Stump", "plant name missing");
		Check(gLawnApp->HasSeedType(type), "custom seed not available");
		{
			SeedChooserScreen chooser;
			chooser.mSeedChooserAge = 0;
			chooser.mMouseVisible = true;
			GLImage chooserImage(gLawnApp->mGLInterface.get());
			chooserImage.Create(800, 600);
			Graphics chooserGraphics(&chooserImage);
			chooser.Draw(&chooserGraphics);
			int x, y;
			chooser.GetSeedPositionInChooser(type, x, y);
			Check(chooser.SeedHitTest(x + 10, y + 10) != type, "custom seed leaked onto vanilla page");
			chooser.MouseDown(400, 100, 1);
			Check(chooser.mPlantPage == 1, "page control failed");
			chooser.Draw(&chooserGraphics);
			Check(chooser.SeedHitTest(x + 10, y + 10) == type, "custom seed hit test failed");
			chooser.ClickedSeedInChooser(chooser.mChosenSeeds[type]);
			chooser.LandFlyingSeed(chooser.mChosenSeeds[type]);
			Check(chooser.FindSeedInBank(0) == type, "selected custom seed missing from bank");
			chooser.ClickedSeedInBank(chooser.mChosenSeeds[type]);
			chooser.LandFlyingSeed(chooser.mChosenSeeds[type]);
			Check(chooser.FindSeedInBank(0) == SEED_NONE, "custom seed deselection failed");
			chooser.MouseDown(400, 100, 1);
			Check(chooser.mPlantPage == 2, "second custom page missing");
			const auto lastType = static_cast<SeedType>(gLawnApp->mModPlants->EndType() - 1);
			chooser.GetSeedPositionInChooser(lastType, x, y);
			Check(chooser.SeedHitTest(x + 10, y + 10) == lastType, "second page hit test failed");
			chooser.Draw(&chooserGraphics);
			chooser.MouseDown(400, 100, 1);
			Check(chooser.mPlantPage == 0, "page control did not wrap");
		}
		Check(board->CanPlantAt(2, 2, type) == PLANTING_OK, "plain ground rejected");
		Check(board->CanPlantAt(0, 2, type) != PLANTING_OK, "Lua placement veto did not reach native checks");
		Check(board->CanPlantAt(-1, 2, type) != PLANTING_OK, "invalid column accepted");
		board->mSunMoney = 100;
		board->mSeedBank->mSeedPackets[0].SetPacketType(type);
		board->mCursorObject->mType = type;
		board->mCursorObject->mImitaterType = SEED_NONE;
		board->mCursorObject->mCursorType = CURSOR_TYPE_PLANT_FROM_BANK;
		board->mCursorObject->mSeedBankIndex = 0;
		board->MouseDownWithPlant(board->GridToPixelX(2, 2) + 40, board->GridToPixelY(2, 2) + 40, 1);
		PlantsOnLawn plants;
		board->GetPlantsOnLawn(2, 2, &plants);
		auto* plant = plants.mNormalPlant;
		Check(plant && plant->mSeedType == type, "normal planting did not create custom plant");
		Check(board->mSunMoney == 50, "planting did not deduct cost");
		Check(board->mSeedBank->mSeedPackets[0].mRefreshing, "planting did not start packet cooldown");
		Check(plant->mPlantHealth == 600 && plant->mPlantMaxHealth == 600, "Lua initialization edits not applied");
		Check(board->CanPlantAt(2, 2, type) != PLANTING_OK, "occupied tile accepted");
		for (int i = 0; i < 500; ++i) plant->Update();
		Check(plant->mFrame == 0 && !plant->mDead, "static plant update failed");
		{
			GLImage image(gLawnApp->mGLInterface.get());
			image.Create(800, 600);
			Graphics graphics(&image);
			plant->Draw(&graphics);
			Plant::DrawSeedType(&graphics, type, SEED_NONE, VARIATION_NORMAL, 100, 100);
			DrawSeedPacket(&graphics, 200, 100, type, SEED_NONE, 0, 255, true, false);
		}
		auto* zombie = board->AddZombieInRow(ZOMBIE_NORMAL, 2, 0);
		zombie->EatPlant(plant);
		Check(plant->mPlantHealth < 600, "native eating did not damage custom plant");
		plant->mPlantHealth = 1;
		zombie->EatPlant(plant);
		Check(plant->mDead, "native eating did not kill custom plant");
		Check(board->CanPlantAt(2, 2, type) == PLANTING_OK, "dead plant still occupies tile");
		auto* another = board->AddPlant(2, 2, type);
		board->MouseDownWithTool(another->mX + 40, another->mY + 40, 1, CURSOR_TYPE_SHOVEL);
		Check(another->mDead && board->mPlantsShoveled == 1, "native shovel did not remove custom plant");
		another->Die(); // removal hook must not fire twice
		auto* vanilla = board->AddPlant(3, 2, SEED_PEASHOOTER);
		Check(vanilla->mPlantHealth == 321, "hook did not cover vanilla initialization");
		vanilla->Die();
		Check(!board->NeedSaveGame(), "experimental board save enabled");
		Check(!LawnSaveGame(board, GetAppDataPath("must-not-exist.v4")), "unsafe save accepted");
		Check(!LawnLoadGame(board, GetAppDataPath("must-not-exist.v4")), "unsafe load accepted");
		Check(gLawnApp->mModRuntime->Diagnostics().empty(), "game hook reported an error");
		std::cout << "plain plant game integration passed (chooser, planting, cost, cooldown, draw, eating, shovel, Lua hooks, isolation, save guards)\n";
		gLawnApp->Shutdown();
		delete gLawnApp;
		gLawnApp = nullptr;
		return 0;
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
