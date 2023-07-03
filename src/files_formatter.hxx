#pragma once

#include "exit_codes.hxx"
#include "statics.hxx"

#include <atomic>
#include <execution>
#include <filesystem>
#include <vector>

int formatFiles(std::vector<std::filesystem::path>&& files)
{
	std::atomic<int> result = RET_OK;

	const auto lambda = [&result](const std::filesystem::path& path)
	{
		if (!abortJob)
		{
			const auto baseCommand = std::string('"' + formatExecPath.generic_string() + '"' + " -i " + path.generic_string());
			const auto fullCommand = baseCommand + " " + formatCommands;
			const int commandRes = std::system(fullCommand.data());
			if (commandRes != RET_OK && result == RET_OK)
			{
				result = RET_CLANG_FORMAT_ERROR;
			}
			++currentFiles;
		}
	};

	if (!abortJob)
		std::for_each(std::execution::par_unseq, files.begin(), files.end(), lambda);

	return result;
}