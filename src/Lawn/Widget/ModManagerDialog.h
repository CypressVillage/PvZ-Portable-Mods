/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODMANAGERDIALOG_H__
#define __MODMANAGERDIALOG_H__

#include "LawnDialog.h"
#include "widget/ListListener.h"
#include <string>
#include <vector>

namespace Sexy
{
	class ScrollbarWidget;
}

class ModListWidget;

class ModManagerDialog : public LawnDialog, public ListListener
{
public:
	ModListWidget* mModList;
	Sexy::ScrollbarWidget* mScrollbar;
	std::vector<bool> mStates;
	std::string mAffectedText;

public:
	ModManagerDialog(LawnApp* theApp);
	virtual ~ModManagerDialog();

	virtual void Resize(int theX, int theY, int theWidth, int theHeight);
	virtual void AddedToManager(WidgetManager* theWidgetManager);
	virtual void RemovedFromManager(WidgetManager* theWidgetManager);
	virtual void Draw(Graphics* g);
	virtual void ButtonDepress(int theId);
	virtual void KeyDown(KeyCode theKey);
	virtual void MouseWheel(int theDelta);
	virtual void ListClicked(int theId, int theIdx, int theClickCount);
	virtual void ListClosed(int) {}
	virtual void ListHiliteChanged(int, int, int) {}

	void ToggleMod(size_t index);
	void EnsureSelectionVisible();
};

#endif
