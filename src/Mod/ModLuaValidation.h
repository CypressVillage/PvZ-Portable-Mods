/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef __MODLUAVALIDATION_H__
#define __MODLUAVALIDATION_H__

#include <string>

bool ModLuaValidateEntry(const std::string& path, const std::string& chunkName, std::string& outError);

#endif
