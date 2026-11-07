// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModDialog.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../../Resources.h"
#include "../../PvzpLib/PvzpStringFile.h"
#include "widget/ListWidget.h"
#include "widget/ScrollbarWidget.h"
#include "widget/WidgetManager.h"
#include "widget/Checkbox.h"
#include "widget/Slider.h"
#include "widget/ScrollbuttonWidget.h"
#include "graphics/SharedImage.h"
#include "graphics/Font.h"
#include "Common.h"
#include <algorithm>

using namespace Sexy;

namespace
{
constexpr int ModList = 201;

class ModScrollbar : public ScrollbarWidget
{
public:
	ModScrollbar(LawnApp* app, int id, ScrollListener* listener)
		: ScrollbarWidget(id, listener),
		mTrack(app->mResourceManager->LoadImage("IMAGE_ZOMBATAR_TOS_SLIDER")),
		mThumb(app->mResourceManager->LoadImage("IMAGE_ZOMBATAR_TOS_SLIDER_THUMB"))
	{
		// Load just these two original images, not the entire Zombatar group.
		// Retain references even if another screen releases that resource group.
		auto* track = static_cast<Image*>(mTrack);
		auto* thumb = static_cast<Image*>(mThumb);
		if (!track || !thumb) throw std::runtime_error("Cannot load the game's vertical slider artwork");
		mAppearance = std::make_unique<Slider>(track, thumb, id, nullptr);
		mAppearance->mHorizontal = false;
		mUpButton->SetVisible(false);
		mDownButton->SetVisible(false);
		mHasTransparencies = true;
	}
	void Draw(Graphics* g) override
	{
		// Reuse the native Slider's tiled track and unscaled original thumb;
		// retain ScrollbarWidget's paging, dragging and list synchronization.
		mAppearance->Resize(0, 0, mWidth, mHeight);
		mAppearance->mVal = mMaxValue > mPageSize ? mValue / (mMaxValue - mPageSize) : 0.0;
		Graphics graphics(*g);
		if (mDisabled) { graphics.SetColorizeImages(true); graphics.SetColor(Color(125, 125, 125)); }
		mAppearance->Draw(&graphics);
	}
	int GetTrackSize() override { return mHeight; }
	int GetThumbSize() override { return std::min(mHeight, mAppearance->mThumbImage->mHeight); }
	int GetThumbPosition() override
	{
		if (mMaxValue <= mPageSize) return 0;
		return static_cast<int>(mValue * (GetTrackSize() - GetThumbSize()) / (mMaxValue - mPageSize) + 0.5);
	}
	void SetThumbPosition(int position) override
	{
		const int travel = GetTrackSize() - GetThumbSize();
		SetValue(travel > 0 ? position * std::max(0.0, mMaxValue - mPageSize) / travel : 0.0);
	}
	void MouseWheel(int delta) override
	{
		if (delta != 0) SetValue(mValue + (delta > 0 ? -5 : 5));
	}
	void ButtonMouseEnter(int) override {}
	void ButtonMouseLeave(int) override {}
	void ButtonMouseMove(int, int, int) override {}
private:
	SharedImageRef mTrack;
	SharedImageRef mThumb;
	std::unique_ptr<Slider> mAppearance;
};

bool HasId(const std::vector<std::string>& ids, const std::string& id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

// Native list rows are single-line; details retain all text in a scrollable pane.
std::string SingleLine(std::string text)
{
	for (auto& c : text) if (static_cast<unsigned char>(c) < 32) c = ' ';
	return text;
}
}

namespace
{
// Keep the game's actual checkbox widget, artwork and input behavior. Wheel
// events over a checkbox must still scroll its containing list.
class ModCheckbox : public Checkbox
{
public:
	ModCheckbox(int index, CheckboxListener* listener, ListWidget* list)
		: Checkbox(IMAGE_OPTIONS_CHECKBOX0, IMAGE_OPTIONS_CHECKBOX1, index, listener), mList(list)
	{
		mHasAlpha = true;
		mHasTransparencies = true;
	}
	void MouseWheel(int delta) override { mList->MouseWheel(delta); }
	bool IsPointVisible(int x, int y) override
	{
		return mX + x >= 4 && mX + x < mList->mWidth - 4 &&
			mY + y >= 4 && mY + y < mList->mHeight - 4 && Checkbox::IsPointVisible(x, y);
	}
	void Draw(Graphics* g) override
	{
		Graphics graphics(*g);
		graphics.ClipRect(4 - mX, 4 - mY, mList->mWidth - 8, mList->mHeight - 8);
		if (mDisabled) { graphics.SetColorizeImages(true); graphics.SetColor(Color(125, 125, 125)); }
		Checkbox::Draw(&graphics);
	}
private:
	ListWidget* mList;
};
}

class ModListWidget : public ListWidget
{
public:
	ModListWidget(_Font* font, ListListener* listener, CheckboxListener* checkListener, size_t count)
		: ListWidget(ModList, font, listener)
	{
		mItemHeight = IMAGE_OPTIONS_CHECKBOX0->mHeight + 4;
		for (size_t i = 0; i < count; ++i)
		{
			AddLine("", false);
			auto checkbox = std::make_unique<ModCheckbox>(static_cast<int>(i), checkListener, this);
			AddWidget(checkbox.get());
			mChecks.push_back(std::move(checkbox));
		}
	}
	~ModListWidget() override { RemoveAllWidgets(); }

	void SetRow(int index, const std::string& title, bool checked, bool disabled, const Color& color)
	{
		SetLine(index, title);
		SetLineColor(index, color);
		mChecks[index]->SetChecked(checked, false);
		mChecks[index]->SetDisabled(disabled);
	}
	void Resize(int x, int y, int width, int height) override
	{
		ListWidget::Resize(x, y, width, height);
		LayoutChecks();
	}
	void ScrollPosition(int id, double position) override
	{
		ListWidget::ScrollPosition(id, position);
		LayoutChecks();
	}
	void Draw(Graphics* g) override
	{
		// The native list has no text inset for child controls. Reserve the right
		// side for native checkboxes and clip/ellipsize titles independently.
		g->SetColor(mColors.mBkg);
		g->FillRect(0, 0, mWidth, mHeight);
		Graphics content(*g);
		content.ClipRect(4, 4, mWidth - 8, mHeight - 8);
		content.SetFont(mFont);
		const int last = std::min(static_cast<int>(mLines.size()) - 1, static_cast<int>(mPosition + mPageSize) + 1);
		for (int i = static_cast<int>(mPosition); i <= last; ++i)
		{
			const int y = 4 + static_cast<int>((i - mPosition) * mItemHeight);
			if (i == mSelectIdx)
			{
				content.SetColor(mColors.mSelect);
				content.FillRect(0, y, mWidth, mItemHeight);
			}
			content.SetColor(i == mHiliteIdx ? mColors.mHilite : i == mSelectIdx ? Color::White : mLineColors[i]);
			auto title = mLines[i];
			const int available = mWidth - IMAGE_OPTIONS_CHECKBOX0->mWidth - 20;
			if (mFont->StringWidth(title) > available)
			{
				do
				{
					size_t end = title.size() - 1;
					while (end > 0 && (static_cast<unsigned char>(title[end]) & 0xc0) == 0x80) --end;
					title.resize(end);
				} while (!title.empty() && mFont->StringWidth(title + "...") > available);
				title += "...";
			}
			content.DrawString(title, 4, y + (mItemHeight - mFont->GetHeight()) / 2 + mFont->GetAscent());
		}
		g->SetColor(mColors.mOutline);
		g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
	}

private:
	std::vector<std::unique_ptr<ModCheckbox>> mChecks;
	void LayoutChecks()
	{
		for (size_t i = 0; i < mChecks.size(); ++i)
		{
			auto& check = mChecks[i];
			const int y = 4 + static_cast<int>((i - mPosition) * mItemHeight) + 2;
			check->Resize(mWidth - IMAGE_OPTIONS_CHECKBOX0->mWidth - 6, y,
				IMAGE_OPTIONS_CHECKBOX0->mWidth, IMAGE_OPTIONS_CHECKBOX0->mHeight);
			// Partly visible rows retain their checkbox; its draw/hit clipping
			// prevents an off-screen portion intercepting clicks outside the list.
			check->SetVisible(y + check->mHeight > 4 && y < mHeight - 4);
		}
	}
};

ModDialog::ModDialog(LawnApp* app)
	: LawnDialog(app, DIALOG_MOD_MANAGER, true, app->GetString("MOD_MANAGER", "Mods"), "", "", Dialog::BUTTONS_OK_CANCEL)
{
	mLawnYesButton->SetLabel(app->GetString("MOD_SAVE", "Save"));
	mLawnNoButton->SetLabel("[DIALOG_BUTTON_CANCEL]");
	try { mSaved = Mod::ReadEnabled(PathFromU8(app->mModConfigPath)); }
	catch (const std::exception& error) { mConfigError = error.what(); }
	mSelection = std::make_unique<Mod::Selection>(Mod::Discover(PathFromU8(GetResourcePath("mods"))), mSaved);
	for (const auto& manifest : mSelection->Installed().manifests)
	{
		std::string error;
		try { Mod::Selection probe(mSelection->Installed(), {}); probe.SetEnabled(manifest.id, true); }
		catch (const std::exception& failure) { error = failure.what(); }
		mRows.push_back({manifest.id, manifest.name, error, &manifest});
	}
	for (const auto& error : mSelection->Installed().errors)
		mRows.push_back({error.modId, error.modId, error.what(), nullptr});
	for (const auto& id : mSaved)
		if (std::none_of(mRows.begin(), mRows.end(), [&](const Row& row) { return row.id == id; }))
			mRows.push_back({id, id, "Enabled Mod is no longer installed.", nullptr});

	mList = std::make_unique<ModListWidget>(FONT_DWARVENTODCRAFT15, this, this, mRows.size());
	mDetails = std::make_unique<ListWidget>(202, FONT_DWARVENTODCRAFT12, nullptr);
	const ListWidgetColorScheme colors{Color(43, 46, 58), Color(107, 109, 145), Color(224, 187, 98),
		Color(255, 242, 160), Color(77, 81, 104), Color::White};
	mList->SetColors(colors);
	mDetails->SetColors(colors);
	mDetails->mColors.mHilite = colors.mText;
	mDetails->mDoFingerWhenHilited = false;
	mDetails->mItemHeight = 22;
	mListScroll = std::make_unique<ModScrollbar>(app, 203, mList.get());
	mDetailScroll = std::make_unique<ModScrollbar>(app, 204, mDetails.get());
	mList->mScrollbar = mListScroll.get();
	mListScroll->SetMaxValue(mRows.size());
	mDetails->mScrollbar = mDetailScroll.get();
	// LawnDialog draws tiled native artwork; round dimensions to whole tiles.
	const int tileWidth = IMAGE_DIALOG_TOPMIDDLE->mWidth;
	const int tileHeight = IMAGE_DIALOG_CENTERLEFT->mHeight;
	const int edges = IMAGE_DIALOG_TOPLEFT->mWidth + IMAGE_DIALOG_TOPRIGHT->mWidth;
	const int caps = IMAGE_DIALOG_TOPLEFT->mHeight + IMAGE_DIALOG_BOTTOMLEFT->mHeight + 45;
	Resize(0, 0, edges + ((740 - edges + tileWidth - 1) / tileWidth) * tileWidth,
		caps + ((550 - caps + tileHeight - 1) / tileHeight) * tileHeight);
	if (!mRows.empty()) mSelected = 0;
	Refresh();
}

ModDialog::~ModDialog() = default;

void ModDialog::AddedToManager(WidgetManager* manager)
{
	LawnDialog::AddedToManager(manager);
	AddWidget(mList.get());
	AddWidget(mDetails.get());
	AddWidget(mListScroll.get());
	AddWidget(mDetailScroll.get());
}

void ModDialog::RemovedFromManager(WidgetManager* manager)
{
	RemoveWidget(mDetailScroll.get());
	RemoveWidget(mListScroll.get());
	RemoveWidget(mDetails.get());
	RemoveWidget(mList.get());
	LawnDialog::RemovedFromManager(manager);
}

void ModDialog::Resize(int x, int y, int width, int height)
{
	LawnDialog::Resize(x, y, width, height);
	// The base constructor calls Resize before these controls exist.
	if (!mList) return;
	const int listWidth = 270, top = 155, paneHeight = height - 280;
	mList->Resize(38, top, listWidth, paneHeight);
	mListScroll->ResizeScrollbar(42 + listWidth, top, 29, paneHeight);
	mDetails->Resize(350, top, width - 420, paneHeight);
	mDetailScroll->ResizeScrollbar(width - 66, top, 29, paneHeight);
	if (mSelected >= 0) ShowDetails();
}

void ModDialog::Draw(Graphics* g)
{
	LawnDialog::Draw(g);
	PvzpDrawString(g, "Installed Mods", 40, 146, FONT_DWARVENTODCRAFT15, Color(224, 187, 98), DS_ALIGN_LEFT);
	PvzpDrawString(g, "Information", 352, 146, FONT_DWARVENTODCRAFT15, Color(224, 187, 98), DS_ALIGN_LEFT);
	const bool pending = mSelection->Enabled() != mApp->mLoadedModIds;
	const std::string status = !mConfigError.empty() ? "Configuration error (see details)" :
		mSelection->Enabled() != mSaved ? "Unsaved changes - Save to apply next launch" :
		pending ? "Saved. Restart the game to apply changes." : "Changes take effect after restarting the game.";
	PvzpDrawStringWrapped(g, status, Rect(40, mHeight - 118, mWidth - 80, 36), FONT_DWARVENTODCRAFT12, Color(224, 187, 98), DS_ALIGN_LEFT);
}

void ModDialog::Refresh()
{
	const double position = mListScroll->mValue;
	for (size_t i = 0; i < mRows.size(); ++i)
	{
		const auto& row = mRows[i];
		const bool enabled = mSelection->IsEnabled(row.id);
		mList->SetRow(static_cast<int>(i), SingleLine(row.name), enabled,
			!mConfigError.empty() || (!enabled && !row.error.empty()),
			row.error.empty() ? mList->mColors.mText : Color(255, 145, 125));
	}
	mList->SetSelect(mSelected);
	mListScroll->SetValue(position);
	mLawnYesButton->SetDisabled(!mConfigError.empty() || mSelection->Enabled() == mSaved);
	ShowDetails();
	MarkDirty();
}

void ModDialog::AddDetail(const std::string& text)
{
	// Wrap at UTF-8 codepoint boundaries, including CJK text and long identifiers.
	std::string line;
	for (size_t i = 0; i < text.size();)
	{
		if (text[i] == '\n') { mDetails->AddLine(line, false); line.clear(); ++i; continue; }
		if (static_cast<unsigned char>(text[i]) < 32) { ++i; continue; }
		size_t end = i + 1;
		while (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80) ++end;
		const auto codepoint = text.substr(i, end - i);
		if (!line.empty() && mDetails->mFont->StringWidth(line + codepoint) > mDetails->mWidth - 12)
		{
			const auto space = line.find_last_of(' ');
			if (space != std::string::npos && space > line.size() / 2)
			{
				mDetails->AddLine(line.substr(0, space), false);
				line.erase(0, space + 1);
			}
			else { mDetails->AddLine(line, false); line.clear(); }
		}
		line += codepoint;
		i = end;
	}
	mDetails->AddLine(line, false);
}

void ModDialog::ShowDetails()
{
	mDetails->RemoveAll();
	mDetailScroll->SetValue(0);
	if (!mConfigError.empty()) AddDetail("Configuration error: " + mConfigError);
	if (mSelected < 0)
	{
		AddDetail("No Mods installed.\n\nPlace Mod folders containing mod.json in the resource directory's mods/ folder, then reopen this screen.");
		return;
	}
	const auto& row = mRows[mSelected];
	AddDetail(row.name);
	AddDetail("ID: " + row.id);
	AddDetail(HasId(mApp->mLoadedModIds, row.id) ? "This session: Loaded" : "This session: Not loaded");
	AddDetail(mSelection->IsEnabled(row.id) ? "Next launch: Enabled" : "Next launch: Disabled");
	if (row.manifest)
	{
		const auto& manifest = *row.manifest;
		AddDetail("Version: " + manifest.version);
		AddDetail("Author: " + (manifest.author.empty() ? "Not specified" : manifest.author));
		AddDetail("API: " + std::to_string(manifest.apiVersion));
		std::string dependencies;
		for (const auto& dependency : manifest.dependencies) dependencies += (dependencies.empty() ? "" : ", ") + dependency;
		AddDetail("Requires: " + (dependencies.empty() ? "None" : dependencies));
		AddDetail("\n" + (manifest.description.empty() ? "No description provided." : manifest.description));
	}
	if (!row.error.empty()) AddDetail("\nUnavailable: " + row.error);
	AddDetail("\nEnabling also enables required Mods. Disabling also disables dependent Mods. Save and restart to apply. Lua scripts are checked at startup.");
}

void ModDialog::ListClicked(int id, int index, int)
{
	if (id != ModList || index < 0 || index >= static_cast<int>(mRows.size())) return;
	mSelected = index;
	Refresh();
	if (index < mListScroll->mValue) mListScroll->SetValue(index);
	else if (index >= mListScroll->mValue + mListScroll->mPageSize)
		mListScroll->SetValue(index - static_cast<int>(mListScroll->mPageSize) + 1);
}

void ModDialog::ButtonDepress(int id)
{
	if (id == Dialog::ID_OK)
	{
		if (!mConfigError.empty() || mSelection->Enabled() == mSaved) return;
		try
		{
			Mod::ResolveOrder(mSelection->Installed(), mSelection->Enabled());
			Mod::SaveEnabled(PathFromU8(mApp->mModConfigPath), mSelection->Enabled());
			mSaved = mSelection->Enabled();
			mLawnNoButton->SetLabel("Close");
			Refresh();
		}
		catch (const std::exception& error)
		{
			mApp->DoDialog(DIALOG_INFO, true, "Cannot save Mods", error.what(), "[DIALOG_BUTTON_OK]", Dialog::BUTTONS_FOOTER);
		}
		return;
	}
	if (id == Dialog::ID_CANCEL)
	{
		auto* app = mApp;
		LawnDialog::ButtonDepress(id);
		if (auto* options = app->GetDialog(DIALOG_NEWOPTIONS)) app->mWidgetManager->SetFocus(options);
	}
}

void ModDialog::CheckboxChecked(int id, bool checked)
{
	if (id < 0 || id >= static_cast<int>(mRows.size())) return;
	mSelected = id;
	const auto& row = mRows[id];
	if (!mConfigError.empty() || (checked && !row.error.empty())) { Refresh(); return; }
	try
	{
		mSelection->SetEnabled(row.id, checked);
		LawnDialog::CheckboxChecked();
		Refresh();
	}
	catch (const std::exception& error)
	{
		Refresh(); // Restore all native checkboxes if validation failed.
		mApp->DoDialog(DIALOG_INFO, true, "Cannot enable Mod", error.what(), "[DIALOG_BUTTON_OK]", Dialog::BUTTONS_FOOTER);
	}
}

void ModDialog::MouseWheel(int delta)
{
	if (!mWidgetManager) return;
	// WidgetManager delivers the wheel to the focused dialog, not the hovered
	// list. Route only inside this dialog, including checkbox/slider children.
	for (WidgetContainer* hovered = mWidgetManager->mOverWidget; hovered && hovered != this; hovered = hovered->mParent)
	{
		if (hovered == mList.get() || hovered == mListScroll.get())
		{
			mList->MouseWheel(delta);
			return;
		}
		if (hovered == mDetails.get() || hovered == mDetailScroll.get())
		{
			mDetails->MouseWheel(delta);
			return;
		}
	}
}

void ModDialog::KeyDown(KeyCode key)
{
	if (key == KEYCODE_ESCAPE) ButtonDepress(Dialog::ID_CANCEL);
	else if (key == KEYCODE_RETURN) ButtonDepress(Dialog::ID_OK);
	else if (key == KEYCODE_SPACE && mSelected >= 0)
		CheckboxChecked(mSelected, !mSelection->IsEnabled(mRows[mSelected].id));
	else if (key == KEYCODE_UP && mSelected > 0) ListClicked(ModList, mSelected - 1, 1);
	else if (key == KEYCODE_DOWN && mSelected + 1 < static_cast<int>(mRows.size())) ListClicked(ModList, mSelected + 1, 1);
}
#endif
