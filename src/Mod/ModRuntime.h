// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ModData.h"
#include <functional>
#include <memory>
#include <optional>

namespace Mod
{
enum class Phase { Empty, Registering, Starting, Running, Saving, Restoring, Stopping, Failed };

// Single owning thread. No game pointers are exposed. Registration is staged
// until every enabled script and startup callback has succeeded.
class Runtime
{
public:
	using Logger = std::function<void(const std::string&, const std::string&)>;
	explicit Runtime(Logger logger = {});
	~Runtime();
	Runtime(const Runtime&) = delete;
	Runtime& operator=(const Runtime&) = delete;
	bool Start(const Discovery& discovery, const std::vector<std::string>& enabled);
	void Stop();
	Phase GetPhase() const;
	const Registry& Contents() const;
	const std::optional<Error>& LastError() const;
	const std::vector<std::string>& Diagnostics() const;
	// Playable, validated definitions created by plants.define (not mod.register).
	const std::map<std::string, Content>& Plants() const;
	std::filesystem::path PlantAsset(const std::string& id, bool card = false) const;
	bool HasGameHooks() const;
	std::string ConfigurationIdentity() const;
	// Game-thread only. Each handler receives the previous handler's edits.
	void Dispatch(const std::string& event, Data& context);
	// Foundation state envelope, NOT a Board/game save. No closures, stacks or
	// numeric runtime handles. The future game-save layer owns atomic disk I/O.
	std::optional<Data> SaveState();
	bool RestoreState(const Data& snapshot);
private:
	struct Impl;
	std::unique_ptr<Impl> mImpl;
};
}
