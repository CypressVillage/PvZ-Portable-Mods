/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __LUAPROXYDIALOG_H__
#define __LUAPROXYDIALOG_H__

#include "../Lawn/Widget/LawnDialog.h"
#include <vector>

struct lua_State;

class LuaProxyDialog : public LawnDialog
{
public:
	struct ButtonEntry
	{
		int id;
		std::string label;
		int luaRef;
	};

	static const int DIALOG_MOD_BASE = 1000;
	static int sNextDialogId;

	LuaProxyDialog(LawnApp* theApp, const std::string& title, const std::string& body, bool modal);
	virtual ~LuaProxyDialog();

	void AddLuaButton(const std::string& text, int luaRef);
	void SetLuaState(lua_State* L);

	virtual void AddedToManager(Sexy::WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void ButtonDepress(int theId);
	virtual void Draw(Sexy::Graphics* g);

	void ReleaseRefs();

	bool mDestroyed;
	bool mInManager;
	lua_State* mLuaState;
	std::vector<ButtonEntry> mButtons;
	std::vector<LawnStoneButton*> mButtonWidgets;
};

#endif
