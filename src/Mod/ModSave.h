/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODSAVE_H__
#define __MODSAVE_H__

#include <string>

namespace ModSave
{
	std::string GetModSavePath(const std::string& modId);
	bool SaveValue(const std::string& modId, const std::string& key, const std::string& value);
	bool LoadValue(const std::string& modId, const std::string& key, std::string& outValue);
}

#endif
