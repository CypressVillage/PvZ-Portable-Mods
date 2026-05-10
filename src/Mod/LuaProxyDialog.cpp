/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "LuaProxyDialog.h"
#include "../LawnApp.h"
#include "../Lawn/Widget/GameButton.h"
#include "../Resources.h"

#if defined(PVZ_ENABLE_LUA)
#include <lua.hpp>
#endif

int LuaProxyDialog::sNextDialogId = LuaProxyDialog::DIALOG_MOD_BASE;

LuaProxyDialog::LuaProxyDialog(LawnApp* theApp, const std::string& title, const std::string& body, bool modal) :
	LawnDialog(theApp, sNextDialogId++, modal, title, body, "", BUTTONS_NONE),
	mDestroyed(false),
	mInManager(false),
	mLuaState(nullptr)
{
	mDrawStandardBack = true;
	CalcSize(0, 0);
}

LuaProxyDialog::~LuaProxyDialog()
{
	ReleaseRefs();
	for (auto* btn : mButtonWidgets)
		delete btn;
	mButtonWidgets.clear();
}

void LuaProxyDialog::AddLuaButton(const std::string& text, int luaRef)
{
	int btnId = 2000 + (int)mButtons.size();
	mButtons.push_back({btnId, text, luaRef});

	auto* btn = MakeButton(btnId, this, text);
	mButtonWidgets.push_back(btn);

	if (mInManager)
		AddWidget(btn);

	CalcSize(0, (int)mButtons.size() * (Sexy::IMAGE_BUTTON_LEFT->mHeight + 8));
}

void LuaProxyDialog::SetLuaState(lua_State* L)
{
	mLuaState = L;
}

void LuaProxyDialog::AddedToManager(Sexy::WidgetManager* theWidgetManager)
{
	LawnDialog::AddedToManager(theWidgetManager);
	mInManager = true;
	for (auto* btn : mButtonWidgets)
		AddWidget(btn);
}

void LuaProxyDialog::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
{
	mInManager = false;
	for (auto* btn : mButtonWidgets)
		RemoveWidget(btn);
	mDestroyed = true;
	ReleaseRefs();
	LawnDialog::RemovedFromManager(theWidgetManager);
}

void LuaProxyDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	LawnDialog::Resize(theX, theY, theWidth, theHeight);

	if (mButtonWidgets.empty())
		return;

	int btnH = Sexy::IMAGE_BUTTON_LEFT->mHeight;
	int btnMinW = Sexy::IMAGE_BUTTON_LEFT->mWidth + Sexy::IMAGE_BUTTON_RIGHT->mWidth;
	int btnAreaW = mWidth - mContentInsets.mLeft - mContentInsets.mRight - mBackgroundInsets.mLeft - mBackgroundInsets.mRight;
	int btnMidW = Sexy::IMAGE_BUTTON_MIDDLE->mWidth;
	int btnExtraW = btnAreaW - btnMinW;
	if (btnExtraW > 0 && btnMidW > 0)
		btnExtraW -= btnExtraW % btnMidW;
	int btnW = btnMinW + (btnExtraW > 0 ? btnExtraW : 0);
	if (btnW > btnAreaW) btnW = btnAreaW;

	int startY = mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - (int)mButtonWidgets.size() * (btnH + 8);
	int startX = mContentInsets.mLeft + mBackgroundInsets.mLeft + (btnAreaW - btnW) / 2;

	for (size_t i = 0; i < mButtonWidgets.size(); i++)
	{
		mButtonWidgets[i]->Resize(startX, startY + (int)i * (btnH + 8), btnW, btnH);
	}
}

void LuaProxyDialog::ButtonDepress(int theId)
{
	if (mUpdateCnt <= mButtonDelay && mButtonDelay >= 0)
		return;

#if defined(PVZ_ENABLE_LUA)
	if (mLuaState)
	{
		for (auto& entry : mButtons)
		{
			if (entry.id == theId && entry.luaRef != LUA_NOREF)
			{
				lua_State* L = static_cast<lua_State*>(mLuaState);
				lua_rawgeti(L, LUA_REGISTRYINDEX, entry.luaRef);
				if (lua_isfunction(L, -1))
				{
					if (lua_pcall(L, 0, 0, 0) != 0)
					{
						TodLog("Lua button callback failed: %s", lua_tostring(L, -1));
						lua_pop(L, 1);
					}
				}
				else
				{
					lua_pop(L, 1);
				}

				LawnApp* app = mApp;
				int dialogId = mId;
				app->KillDialog(dialogId);
				return;
			}
		}
	}
#endif
}

void LuaProxyDialog::Draw(Sexy::Graphics* g)
{
	LawnDialog::Draw(g);
}

void LuaProxyDialog::ReleaseRefs()
{
#if defined(PVZ_ENABLE_LUA)
	if (mLuaState)
	{
		lua_State* L = static_cast<lua_State*>(mLuaState);
		for (auto& entry : mButtons)
		{
			if (entry.luaRef != LUA_NOREF)
			{
				luaL_unref(L, LUA_REGISTRYINDEX, entry.luaRef);
				entry.luaRef = LUA_NOREF;
			}
		}
	}
#endif
}
