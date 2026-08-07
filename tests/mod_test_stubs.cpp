#include "Mod/ModRegistry.h"
#include "SexyAppFramework/Common.h"

#include <cstdarg>
#include <filesystem>

namespace
{
	std::filesystem::path gAppDataFolder;
	std::filesystem::path gResourceFolder;
}

namespace Sexy
{
	void SetAppDataFolder(const std::string& path)
	{
		gAppDataFolder = PathFromU8(path);
	}

	std::string GetAppDataPath(const std::string& relativePath)
	{
		return PathToU8(gAppDataFolder / PathFromU8(relativePath));
	}

	void SetResourceFolder(const std::string& path)
	{
		gResourceFolder = PathFromU8(path);
	}

	std::string GetResourcePath(const std::string& relativePath)
	{
		return PathToU8(gResourceFolder / PathFromU8(relativePath));
	}

	void MkDir(const std::string& dir)
	{
		std::error_code error;
		std::filesystem::create_directories(PathFromU8(dir), error);
	}
}

void TodLog(const char*, ...)
{
}

ModRegistry gModRegistry;

void ModRegistry::InjectPlantStringOverrides(Sexy::SexyAppBase*)
{
}
