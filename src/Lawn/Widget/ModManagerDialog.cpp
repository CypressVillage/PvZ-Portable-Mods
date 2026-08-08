/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "ModManagerDialog.h"
#include "../../LawnApp.h"
#include "../../Mod/ModLoader.h"
#include "../../Resources.h"
#include "../../Sexy.TodLib/TodCommon.h"
#include "GameButton.h"
#include "graphics/Font.h"
#include "widget/ListWidget.h"
#include "widget/ScrollbarWidget.h"
#include <algorithm>

using namespace Sexy;

namespace
{
	static int gModListColors[][3] = {
		{23, 24, 35},
		{0, 0, 0},
		{235, 225, 180},
		{255, 255, 255},
		{55, 95, 45},
		{255, 255, 255}
	};
}

class ModScrollbarWidget : public ScrollbarWidget
{
public:
	ModScrollbarWidget(int theId, ScrollListener* theScrollListener)
		: ScrollbarWidget(theId, theScrollListener)
	{
	}

	void ButtonMouseEnter(int) override {}
	void ButtonMouseLeave(int) override {}
	void ButtonMouseMove(int, int, int) override {}
};

class ModListWidget : public ListWidget
{
public:
	ModManagerDialog* mDialog;

	ModListWidget(ModManagerDialog* theDialog)
		: ListWidget(0, FONT_DWARVENTODCRAFT15, theDialog), mDialog(theDialog)
	{
	}

	void Draw(Graphics* g) override
	{
		Graphics clip(*g);
		clip.ClipRect(0, 0, mWidth, mHeight);
		clip.SetFont(mFont);
		const std::vector<ModInfo>& mods = gModLoader.GetMods();
		int firstLine = static_cast<int>(mPosition);
		int lastLine = std::min(static_cast<int>(mods.size()) - 1, firstLine + static_cast<int>(mPageSize) + 1);
		for (int i = firstLine; i <= lastLine; i++)
		{
			int y = 4 + static_cast<int>((i - mPosition) * mItemHeight);
			Image* checkbox = mods[i].valid && mDialog->mStates[i] ? IMAGE_OPTIONS_CHECKBOX1 : IMAGE_OPTIONS_CHECKBOX0;
			constexpr float checkboxScale = 0.60f;
			float checkboxY = y + (mItemHeight - checkbox->GetHeight() * checkboxScale) / 2.0f;
			TodDrawImageScaledF(&clip, checkbox, 2.0f, checkboxY, checkboxScale, checkboxScale);

			if (!mods[i].valid)
				clip.SetColor(Color(220, 75, 65));
			else if (i == mSelectIdx)
				clip.SetColor(Color(245, 225, 150));
			else if (i == mHiliteIdx)
				clip.SetColor(Color(145, 150, 190));
			else
				clip.SetColor(Color(107, 109, 145));
			std::string name = mLines[i];
			const std::string ellipsis = "...";
			int maxNameWidth = mWidth - 54;
			while (!name.empty() && mFont->StringWidth(name + ellipsis) > maxNameWidth)
			{
				size_t lastCodepoint = name.size() - 1;
				while (lastCodepoint > 0 && (static_cast<unsigned char>(name[lastCodepoint]) & 0xC0) == 0x80)
					lastCodepoint--;
				name.erase(lastCodepoint);
			}
			if (name != mLines[i]) name += ellipsis;
			clip.DrawString(name, 34, y + (mItemHeight - mFont->GetHeight()) / 2 + mFont->GetAscent());
		}
	}

	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override
	{
		int index = static_cast<int>(((y - 4) / static_cast<double>(mItemHeight)) + mPosition);
		if (index >= 0 && index < GetLineCount())
		{
			SetSelect(index);
			if (x <= 32)
				mDialog->ToggleMod(static_cast<size_t>(index));
		}
		ListWidget::MouseDown(x, y, theBtnNum, theClickCount);
	}
};

ModManagerDialog::ModManagerDialog(LawnApp* theApp)
	: LawnDialog(theApp, Dialogs::DIALOG_MOD_MANAGER, true, "Mod Manager", "", "", Dialog::BUTTONS_OK_CANCEL)
{
	mVerticalCenterText = false;
	mTallBottom = false;
	mLawnYesButton->SetLabel("Apply");
	mModList = new ModListWidget(this);
	mModList->SetColors(gModListColors, LENGTH(gModListColors));
	mModList->mItemHeight = 30;
	mModList->mDrawOutline = false;
	mModList->mDoFingerWhenHilited = true;
	mScrollbar = new ModScrollbarWidget(1, mModList);
	mScrollbar->SetInvisIfNoScroll(true);
	mModList->mScrollbar = mScrollbar;

	const std::vector<ModInfo>& mods = gModLoader.GetMods();
	for (const ModInfo& mod : mods)
	{
		std::string name = mod.manifest.name.empty() ? (mod.manifest.id.empty() ? mod.directoryName : mod.manifest.id) : mod.manifest.name;
		mModList->AddLine(name, false);
		mStates.push_back(mod.enabled);
		if (!mod.valid)
			mModList->SetLineColor(mModList->GetLineCount() - 1, Color(235, 90, 75));
	}
	if (!mods.empty())
		mModList->SetSelect(0);
	CalcSize(475, 335);
}

ModManagerDialog::~ModManagerDialog()
{
	delete mModList;
	delete mScrollbar;
}

void ModManagerDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);
	int top = GetTop() - 28;
	int contentHeight = mHeight - top - 84;
	int listWidth = 235;
	mModList->Resize(GetLeft(), top, listWidth, contentHeight);
	mScrollbar->ResizeScrollbar(GetLeft() + listWidth - 18, top + 1, 17, contentHeight - 2);
}

void ModManagerDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	AddWidget(mModList);
	AddWidget(mScrollbar);
}

void ModManagerDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	LawnDialog::RemovedFromManager(theWidgetManager);
	RemoveWidget(mModList);
	RemoveWidget(mScrollbar);
}

void ModManagerDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);
	const std::vector<ModInfo>& mods = gModLoader.GetMods();
	int selected = mModList->mSelectIdx;
	int x = GetLeft() + 255;
	int y = GetTop() - 28;
	int width = GetWidth() - 255;
	int height = mHeight - y - 84;
	g->SetFont(FONT_BRIANNETOD16);
	if (selected < 0 || selected >= static_cast<int>(mods.size()))
	{
		g->SetColor(Color(235, 225, 180));
		g->DrawString("No Mods found", x + 12, y + 28);
		return;
	}

	const ModInfo& mod = mods[selected];
	std::string name = mod.manifest.name.empty() ? (mod.manifest.id.empty() ? mod.directoryName : mod.manifest.id) : mod.manifest.name;
	g->SetColor(mod.valid ? Color(255, 245, 200) : Color(255, 100, 85));
	g->DrawString(name, x + 12, y + 25);
	g->SetFont(FONT_DWARVENTODCRAFT12);
	g->SetColor(Color(190, 190, 170));
	int lineY = y + 48;
	auto drawLine = [&](const std::string& label, const std::string& value) {
		if (!value.empty())
		{
			g->DrawString(label + value, x + 12, lineY);
			lineY += 19;
		}
	};
	drawLine("ID: ", mod.manifest.id.empty() ? "(invalid manifest)" : mod.manifest.id);
	drawLine("Version: ", mod.manifest.version);
	drawLine("Author: ", mod.manifest.author);
	if (!mod.manifest.dependencies.empty())
	{
		std::string dependencies;
		for (const std::string& dependency : mod.manifest.dependencies)
		{
			if (!dependencies.empty()) dependencies += ", ";
			dependencies += dependency;
		}
		drawLine("Dependencies: ", dependencies);
	}
	std::string status;
	if (!mod.valid)
		status = "Error";
	else if (mStates[selected] != mod.loaded)
		status = mStates[selected] ? "Enabled after restart" : "Disabled after restart";
	else
		status = mod.loaded ? "Enabled" : "Disabled";
	drawLine("Status: ", status);

	Graphics detail(*g);
	detail.ClipRect(x + 8, lineY, width - 16, y + height - lineY - 8);
	detail.SetFont(FONT_DWARVENTODCRAFT12);
	if (!mod.error.empty())
	{
		detail.SetColor(Color(255, 100, 85));
		lineY += detail.WriteWordWrapped(Rect(x + 12, lineY, width - 24, 0), mod.error, 16, DS_ALIGN_LEFT) + 8;
	}
	if (!mAffectedText.empty())
	{
		detail.SetColor(Color(255, 220, 100));
		lineY += detail.WriteWordWrapped(Rect(x + 12, lineY, width - 24, 0), mAffectedText, 16, DS_ALIGN_LEFT) + 8;
	}
	if (!mod.manifest.description.empty())
	{
		detail.SetColor(Color(235, 225, 180));
		detail.WriteWordWrapped(Rect(x + 12, lineY, width - 24, 0), mod.manifest.description, 16, DS_ALIGN_LEFT);
	}
}

void ModManagerDialog::ToggleMod(size_t index)
{
	if (index >= mStates.size())
		return;
	std::vector<size_t> affected;
	std::string error;
	if (!gModLoader.SetEnabledState(mStates, index, !mStates[index], affected, error))
	{
		mAffectedText = error;
		MarkDirty();
		return;
	}
	mAffectedText.clear();
	if (affected.size() > 1)
	{
		mAffectedText = "Also changed: ";
		const std::vector<ModInfo>& mods = gModLoader.GetMods();
		bool first = true;
		for (size_t changed : affected)
		{
			if (changed == index) continue;
			if (!first) mAffectedText += ", ";
			mAffectedText += mods[changed].manifest.name.empty() ? mods[changed].manifest.id : mods[changed].manifest.name;
			first = false;
		}
	}
	mModList->MarkDirty();
	MarkDirty();
}

void ModManagerDialog::ListClicked(int theId, int theIdx, int theClickCount)
{
	(void)theId;
	(void)theClickCount;
	mModList->SetSelect(theIdx);
	MarkDirty();
}

void ModManagerDialog::EnsureSelectionVisible()
{
	if (mModList->mSelectIdx < mModList->mPosition)
		mScrollbar->SetValue(mModList->mSelectIdx);
	else if (mModList->mSelectIdx >= mModList->mPosition + mModList->mPageSize)
		mScrollbar->SetValue(mModList->mSelectIdx - static_cast<int>(mModList->mPageSize) + 1);
}

void ModManagerDialog::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_ESCAPE)
	{
		mApp->KillDialog(mId);
		return;
	}
	if (mStates.empty())
	{
		LawnDialog::KeyDown(theKey);
		return;
	}
	if (theKey == KEYCODE_UP || theKey == KEYCODE_DOWN)
	{
		int delta = theKey == KEYCODE_UP ? -1 : 1;
		mModList->SetSelect(std::clamp(mModList->mSelectIdx + delta, 0, static_cast<int>(mStates.size()) - 1));
		EnsureSelectionVisible();
		return;
	}
	if (theKey == KEYCODE_SPACE)
	{
		ToggleMod(static_cast<size_t>(mModList->mSelectIdx));
		return;
	}
	LawnDialog::KeyDown(theKey);
}

void ModManagerDialog::MouseWheel(int theDelta)
{
	mModList->MouseWheel(theDelta);
}

void ModManagerDialog::ButtonDepress(int theId)
{
	if (theId == Dialog::ID_CANCEL)
	{
		mApp->KillDialog(mId);
		return;
	}
	if (theId == Dialog::ID_OK)
	{
		std::string error;
		if (!gModLoader.SaveEnabledStates(mStates, error))
		{
			mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "Mod Manager", error.c_str(), "[DIALOG_BUTTON_OK]", "", Dialog::BUTTONS_FOOTER);
			return;
		}
		mApp->LawnMessageBox(Dialogs::DIALOG_MESSAGE, "Mods Updated", "Changes will take effect after restarting the game.", "[DIALOG_BUTTON_OK]", "", Dialog::BUTTONS_FOOTER);
		mApp->KillDialog(mId);
		return;
	}
	LawnDialog::ButtonDepress(theId);
}
