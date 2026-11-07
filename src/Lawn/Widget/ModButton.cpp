// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModButton.h"
#include "../Board.h"
#include "../../LawnApp.h"

ModButton::ModButton(Board& board, const Mod::Runtime::Button& definition)
	: LawnStoneButton(nullptr, 0, this), mBoard(board), mModId(definition.id)
{
	SetLabel(definition.text);
	mTranslateX = mTranslateY = 1;
	mHasAlpha = mHasTransparencies = true;
	Resize(800 - definition.right - definition.width, definition.top, definition.width, definition.height);
	mVisible = false;
}

void ModButton::Update()
{
	LawnStoneButton::Update();
	auto* runtime = mBoard.mApp->mModRuntime.get();
	const bool playing = runtime && runtime->GetPhase() == Mod::Phase::Running &&
		mBoard.mApp->mGameScene == GameScenes::SCENE_PLAYING && !mBoard.mLevelComplete;
	SetVisible(playing);
	SetDisabled(!playing || mBoard.mPaused || !mBoard.CanInteractWithBoardButtons() || mBoard.mIgnoreMouseUp);
	if (runtime) for (const auto& button : runtime->Buttons())
		if (button.id == mModId && button.text != mLabel) { SetLabel(button.text); MarkDirty(); }
}

void ModButton::ButtonDepress(int)
{
	if (!mDisabled && mVisible && !mBoard.mPaused && mBoard.mApp->mModRuntime)
		mBoard.mApp->mModRuntime->ClickButton(mModId);
}
#endif
