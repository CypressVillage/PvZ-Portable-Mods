/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODCONFIG_H__
#define __MODCONFIG_H__

#include <set>
#include <string>

namespace ModConfig
{
	using DisabledIds = std::set<std::string>;

	std::string GetPath();
	bool LoadDisabledIds(DisabledIds& outIds, std::string& outError);
	bool SaveDisabledIds(const DisabledIds& ids, std::string& outError);
}

#endif
