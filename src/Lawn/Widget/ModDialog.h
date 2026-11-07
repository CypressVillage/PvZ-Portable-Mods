// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD

#include "LawnDialog.h"
#include "../../Mod/ModData.h"
#include "widget/ListListener.h"
#include "widget/CheckboxListener.h"

namespace Sexy { class ListWidget; class ScrollbarWidget; }
class ModListWidget;

class ModDialog : public LawnDialog, public Sexy::ListListener, public Sexy::CheckboxListener
{
public:
	explicit ModDialog(LawnApp* app);
	~ModDialog() override;
	void AddedToManager(Sexy::WidgetManager* manager) override;
	void RemovedFromManager(Sexy::WidgetManager* manager) override;
	void Resize(int x, int y, int width, int height) override;
	void Draw(Sexy::Graphics* g) override;
	void ButtonDepress(int id) override;
	void KeyDown(Sexy::KeyCode key) override;
	void MouseWheel(int delta) override;
	void ListClicked(int id, int index, int count) override;
	void ListClosed(int) override {}
	void ListHiliteChanged(int, int, int) override {}
	void CheckboxChecked(int id, bool checked) override;

private:
	struct Row { std::string id, name, error; const Mod::Manifest* manifest = nullptr; };
	std::unique_ptr<Mod::Selection> mSelection;
	std::vector<std::string> mSaved;
	std::vector<Row> mRows;
	std::string mConfigError;
	std::unique_ptr<ModListWidget> mList;
	std::unique_ptr<Sexy::ListWidget> mDetails;
	std::unique_ptr<Sexy::ScrollbarWidget> mListScroll;
	std::unique_ptr<Sexy::ScrollbarWidget> mDetailScroll;
	int mSelected = -1;
	void Refresh();
	void ShowDetails();
	void AddDetail(const std::string& text);
};
#endif
