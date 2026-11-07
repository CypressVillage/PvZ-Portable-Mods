// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD

#include <nlohmann/json.hpp>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace Mod
{
using Data = nlohmann::json;
inline constexpr int ApiVersion = 1;

struct Error : std::runtime_error
{
	std::string modId;
	std::string phase;
	Error(std::string id, std::string stage, std::string message);
};

bool ValidName(const std::string& name);
// Only regular files below root are readable; canonical paths also check symlinks.
std::filesystem::path ResolveFile(const std::filesystem::path& root, const std::string& relative);
std::string ReadText(const std::filesystem::path& path);
Data ReadJson(const std::filesystem::path& path);

struct Manifest
{
	std::string id;
	std::string name;
	std::string version;
	std::string author;
	std::string description;
	int apiVersion = 0;
	std::string entry;
	std::vector<std::string> dependencies;
	std::filesystem::path directory;
	static Manifest Load(const std::filesystem::path& directory);
};

struct Discovery
{
	std::vector<Manifest> manifests;
	std::vector<Error> errors;
};
Discovery Discover(const std::filesystem::path& root);
// Input is enabled IDs in player priority order, low to high. Dependencies win
// over that preference; lexical ID breaks any remaining tie. Never auto-enables.
std::vector<Manifest> ResolveOrder(const Discovery& discovery, const std::vector<std::string>& enabled);

// Staged UI changes never modify the running interpreter or player profile.
class Selection
{
public:
	Selection(Discovery discovery, std::vector<std::string> enabled);
	const Discovery& Installed() const { return mDiscovery; }
	const std::vector<std::string>& Enabled() const { return mEnabled; }
	bool IsEnabled(const std::string& id) const;
	// Enables required dependencies, or disables all transitive dependents.
	// A failed enable leaves the selection unchanged.
	void SetEnabled(const std::string& id, bool enabled);
private:
	Discovery mDiscovery;
	std::vector<std::string> mEnabled;
};

std::vector<std::string> ReadEnabled(const std::filesystem::path& path);
// Preserves other configuration fields and replaces the file atomically.
void SaveEnabled(const std::filesystem::path& path, const std::vector<std::string>& enabled);

struct Content
{
	std::string id;
	std::string kind;
	std::string owner;
	Data definition;
};

class Registry
{
public:
	void Add(const std::string& owner, const std::string& kind, const std::string& name, Data definition);
	const Content* Find(const std::string& id) const;
	const std::map<std::string, Content>& Entries() const { return mEntries; }
	void Clear() { mEntries.clear(); }
private:
	std::map<std::string, Content> mEntries;
};
}
#endif
