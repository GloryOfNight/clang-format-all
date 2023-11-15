#pragma once

#include "exit_codes.hxx"
#include "statics.hxx"

#include <algorithm>
#include <atomic>
#include <execution>
#include <filesystem>
#include <vector>

std::vector<std::filesystem::path> collectFilepaths(const std::filesystem::path& dir, std::vector<std::filesystem::path>&& ignorePaths)
{
	std::vector<std::filesystem::path> files;
	files.reserve(4096);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filePath = entry.path();

		const bool isCxxExtension = std::find(extensions.begin(), extensions.end(), filePath.filename().extension().generic_string()) != extensions.end();
		if (!isCxxExtension)
			continue;

		bool isIgnored = false;

		for (auto igDir : ignorePaths)
		{
			if (std::filesystem::is_directory(igDir))
			{
				const auto igDirStr = igDir.generic_string();
				auto dirStr = filePath.generic_string();
				if (const auto pos = dirStr.find_first_of('/', igDirStr.size() - 1); pos != std::string::npos)
					dirStr.erase(pos);
				isIgnored = dirStr == igDirStr;
			}
			else if (std::filesystem::is_regular_file(igDir) && igDir == filePath)
			{
				isIgnored = true;
			}

			if (isIgnored)
			{
				log(VERBOSE, "Ignoring file: %s", filePath.generic_string().data());
				break;
			}
		}

		if (isIgnored)
			continue;

		files.push_back(filePath);
		totalFiles = files.size();
	}
	return files;
}

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