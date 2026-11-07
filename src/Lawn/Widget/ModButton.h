// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD
#include "GameButton.h"
#include "widget/ButtonListener.h"
#include "Mod/ModRuntime.h"

class Board;
// A native child widget: input is consumed before reaching the lawn.
class ModButton : public LawnStoneButton, public Sexy::ButtonListener
{
public:
	ModButton(Board& board, const Mod::Runtime::Button& definition);
	void Update() override;
	void ButtonDepress(int) override;
	void ButtonPress(int) override {}
	void ButtonDownTick(int) override {}
	void ButtonMouseEnter(int) override {}
	void ButtonMouseLeave(int) override {}
	void ButtonMouseMove(int, int, int) override {}
private:
	Board& mBoard;
	std::string mModId;
};
#endif
