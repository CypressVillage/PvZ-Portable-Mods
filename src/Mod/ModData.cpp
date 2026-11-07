// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModData.h"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <functional>
#include <chrono>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Mod
{
Error::Error(std::string id, std::string stage, std::string message)
	: std::runtime_error("[mod " + id + "][" + stage + "] " + message), modId(std::move(id)), phase(std::move(stage)) {}

bool ValidName(const std::string& name)
{
	if (name.empty() || name.size() > 96 || name.front() < 'a' || name.front() > 'z') return false;
	return std::all_of(name.begin(), name.end(), [](char c) {
		return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
	}) && name.find("..") == std::string::npos && name.back() != '.';
}

std::filesystem::path ResolveFile(const std::filesystem::path& root, const std::string& relative)
{
	if (relative.empty() || relative.find('\0') != std::string::npos || relative.find('\\') != std::string::npos ||
		relative.find(':') != std::string::npos) throw std::runtime_error("invalid relative path: " + relative);
	auto path = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(relative.data()), relative.size()));
	if (path.is_absolute() || path.has_root_path()) throw std::runtime_error("absolute path is forbidden");
	for (const auto& part : path)
		if (part == ".." || part == ".") throw std::runtime_error("path traversal is forbidden");
	auto base = std::filesystem::canonical(root);
	auto resolved = std::filesystem::canonical(base / path);
	auto b = base.begin(), p = resolved.begin();
	for (; b != base.end(); ++b, ++p)
		if (p == resolved.end() || *b != *p) throw std::runtime_error("path escapes mod directory");
	if (!std::filesystem::is_regular_file(resolved)) throw std::runtime_error("not a regular file: " + relative);
	return resolved;
}

std::string ReadText(const std::filesystem::path& path)
{
	std::ifstream stream(path, std::ios::binary);
	if (!stream) throw std::runtime_error("cannot read " + path.string());
	std::ostringstream text;
	text << stream.rdbuf();
	if (stream.bad()) throw std::runtime_error("read failed: " + path.string());
	return text.str();
}

Data ReadJson(const std::filesystem::path& path)
{
	// Reject duplicate keys instead of silently accepting the last definition.
	std::vector<std::set<std::string>> keys;
	return Data::parse(ReadText(path), [&keys](int, Data::parse_event_t event, Data& value) {
		if (event == Data::parse_event_t::object_start) keys.emplace_back();
		if (event == Data::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
			throw std::runtime_error("duplicate JSON key: " + value.get<std::string>());
		if (event == Data::parse_event_t::object_end) keys.pop_back();
		return true;
	});
}

Manifest Manifest::Load(const std::filesystem::path& directory)
{
	std::string id = directory.filename().string();
	try
	{
		auto data = ReadJson(ResolveFile(directory, "mod.json"));
		if (!data.is_object()) throw std::runtime_error("manifest must be an object");
		Manifest result;
		result.directory = std::filesystem::canonical(directory);
		result.id = data.at("id").get<std::string>();
		if (!ValidName(result.id) || result.id == "base") throw std::runtime_error("invalid/reserved mod id");
		id = result.id;
		result.name = data.at("name").get<std::string>();
		result.version = data.at("version").get<std::string>();
		result.author = data.value("author", std::string());
		result.description = data.value("description", std::string());
		if (result.name.empty() || result.version.empty()) throw std::runtime_error("name/version must not be empty");
		if (!data.at("api_version").is_number_integer()) throw std::runtime_error("api_version must be an integer");
		if (data.at("api_version") < 0 || data.at("api_version") > 2147483647)
			throw std::runtime_error("api_version is out of range");
		result.apiVersion = data.at("api_version").get<int>();
		result.entry = data.value("entry", std::string("main.lua"));
		ResolveFile(directory, result.entry);
		if (data.contains("requires")) result.dependencies = data.at("requires").get<std::vector<std::string>>();
		std::set<std::string> seen;
		for (const auto& dependency : result.dependencies)
			if (!ValidName(dependency) || dependency == result.id || !seen.insert(dependency).second)
				throw std::runtime_error("invalid, self or duplicate dependency: " + dependency);
		return result;
	}
	catch (const std::exception& error) { throw Error(id, "manifest", error.what()); }
}

Discovery Discover(const std::filesystem::path& root)
{
	Discovery result;
	try
	{
		if (!std::filesystem::exists(root)) return result;
		std::vector<std::filesystem::path> directories;
		for (const auto& entry : std::filesystem::directory_iterator(root))
			if (entry.is_directory()) directories.push_back(entry.path());
		std::sort(directories.begin(), directories.end());
		for (const auto& directory : directories)
		{
			try { result.manifests.push_back(Manifest::Load(directory)); }
			catch (const Error& error) { result.errors.push_back(error); }
		}
	}
	catch (const std::exception& error) { result.errors.emplace_back("discovery", "discover", error.what()); }
	return result;
}

std::vector<Manifest> ResolveOrder(const Discovery& discovery, const std::vector<std::string>& enabled)
{
	std::map<std::string, const Manifest*> available;
	std::set<std::string> duplicate;
	for (const auto& manifest : discovery.manifests)
		if (!available.emplace(manifest.id, &manifest).second) duplicate.insert(manifest.id);
	std::map<std::string, size_t> priority;
	for (size_t i = 0; i < enabled.size(); ++i)
	{
		const auto& id = enabled[i];
		if (!ValidName(id) || !priority.emplace(id, i).second) throw Error(id, "resolve", "invalid/duplicate enabled id");
		if (duplicate.count(id)) throw Error(id, "resolve", "duplicate installed mod id");
		for (const auto& error : discovery.errors)
			if (error.modId == id || error.phase == "discover") throw error;
		if (!available.count(id)) throw Error(id, "resolve", "enabled mod is missing or has an invalid manifest");
		if (available.at(id)->apiVersion != ApiVersion) throw Error(id, "resolve", "unsupported framework API version");
	}
	for (const auto& id : enabled)
		for (const auto& dependency : available.at(id)->dependencies)
			if (!priority.count(dependency)) throw Error(id, "resolve", "required mod is missing or disabled: " + dependency);
	std::vector<Manifest> result;
	std::set<std::string> done;
	while (result.size() != enabled.size())
	{
		const Manifest* next = nullptr;
		for (const auto& id : enabled)
		{
			const auto* candidate = available.at(id);
			if (done.count(id) || !std::all_of(candidate->dependencies.begin(), candidate->dependencies.end(),
				[&done](const auto& dep) { return done.count(dep) != 0; })) continue;
			if (!next || priority.at(id) < priority.at(next->id)) next = candidate;
		}
		if (!next)
		{
			std::string unresolved;
			for (const auto& id : enabled) if (!done.count(id)) unresolved += " " + id;
			throw Error("dependencies", "resolve", "dependency cycle; blocked mods:" + unresolved);
		}
		done.insert(next->id);
		result.push_back(*next);
	}
	return result;
}

Selection::Selection(Discovery discovery, std::vector<std::string> enabled)
	: mDiscovery(std::move(discovery)), mEnabled(std::move(enabled)) {}

bool Selection::IsEnabled(const std::string& id) const
{
	return std::find(mEnabled.begin(), mEnabled.end(), id) != mEnabled.end();
}

void Selection::SetEnabled(const std::string& id, bool enabled)
{
	auto candidate = mEnabled;
	if (enabled)
	{
		std::set<std::string> visiting, done;
		std::function<void(const std::string&)> add = [&](const std::string& current) {
			if (done.count(current)) return;
			if (!visiting.insert(current).second) throw Error(current, "resolve", "dependency cycle");
			const Manifest* found = nullptr;
			for (const auto& manifest : mDiscovery.manifests)
				if (manifest.id == current)
				{
					if (found) throw Error(current, "resolve", "duplicate installed mod id");
					found = &manifest;
				}
			if (!found) throw Error(current, "resolve", "required mod is missing or invalid");
			if (found->apiVersion != ApiVersion) throw Error(current, "resolve", "unsupported framework API version");
			for (const auto& dependency : found->dependencies) add(dependency);
			if (std::find(candidate.begin(), candidate.end(), current) == candidate.end()) candidate.push_back(current);
			visiting.erase(current);
			done.insert(current);
		};
		add(id);
		ResolveOrder(mDiscovery, candidate);
	}
	else
	{
		std::set<std::string> removed{id};
		bool changed;
		do
		{
			changed = false;
			for (const auto& manifest : mDiscovery.manifests)
				if (!removed.count(manifest.id) && std::any_of(manifest.dependencies.begin(), manifest.dependencies.end(),
					[&](const auto& dependency) { return removed.count(dependency) != 0; }))
					changed |= removed.insert(manifest.id).second;
		} while (changed);
		std::erase_if(candidate, [&](const auto& current) { return removed.count(current) != 0; });
	}
	mEnabled = std::move(candidate);
}

std::vector<std::string> ReadEnabled(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path)) return {};
	return ReadJson(path).at("enabled").get<std::vector<std::string>>();
}

void SaveEnabled(const std::filesystem::path& path, const std::vector<std::string>& enabled)
{
	auto config = std::filesystem::exists(path) ? ReadJson(path) : Data::object();
	config["enabled"] = enabled;
	const auto text = config.dump(2) + "\n";
	std::filesystem::create_directories(path.parent_path());
	auto temporary = path;
	temporary += ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	try
	{
		std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
		stream.exceptions(std::ios::failbit | std::ios::badbit);
		stream.write(text.data(), static_cast<std::streamsize>(text.size()));
		stream.close();
#ifdef _WIN32
		if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
			throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "cannot replace mod configuration");
#else
		std::filesystem::rename(temporary, path);
#endif
	}
	catch (...)
	{
		std::error_code ignored;
		std::filesystem::remove(temporary, ignored);
		throw;
	}
}

void Registry::Add(const std::string& owner, const std::string& kind, const std::string& name, Data definition)
{
	static const std::set<std::string> kinds = {"plant", "zombie", "projectile", "level", "ability", "image", "sound", "particle", "reanim"};
	if (!ValidName(owner) || owner == "base" || !ValidName(name)) throw std::runtime_error("invalid/reserved content identity");
	if (!kinds.count(kind)) throw std::runtime_error("unknown content kind: " + kind);
	if (!definition.is_object()) throw std::runtime_error("content definition must be a data object");
	std::string id = owner + ":" + name;
	if (!mEntries.emplace(id, Content{id, kind, owner, std::move(definition)}).second)
		throw std::runtime_error("duplicate content id: " + id);
}

const Content* Registry::Find(const std::string& id) const
{
	auto found = mEntries.find(id);
	return found == mEntries.end() ? nullptr : &found->second;
}
}
#endif
