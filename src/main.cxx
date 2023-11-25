#include "log.hxx"
#include "statics.hxx"
#include "types.hxx"

#include <array>
#include <atomic>
#include <csignal>
#include <cstdarg>
#include <execution>
#include <filesystem>
#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

using pathsArrayTuple = std::tuple<std::vector<std::filesystem::path>, std::vector<std::filesystem::path>>;

void handleAbort(int sig);				// handle abort signal from terminal or system
void parseArgs(int argc, char* argv[]); // parse argument list
void parseEnvp(char* envp[]);			// look and parse environment variables we could use

pathsArrayTuple collectFilepaths(const std::filesystem::path& dir, const std::vector<std::filesystem::path>& ignorePaths); // find and collect all files that would be ignored and formatted
int formatFiles(const std::vector<std::filesystem::path>& files);														   // format provided files with clang-format

int main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	parseArgs(argc, argv);
	parseEnvp(envp);

	if (printHelp)
	{
		CF_LOG(Display, "Available arguments list:");
		for (auto& arg : args)
		{
			if (arg.note_help.size() == 0)
				continue;

			CF_LOG(Display, arg.note_help);
		}

		CF_LOG(Display, "\nSource code page: https://github.com/GloryOfNight/clang-format-all.git");
		CF_LOG(Display, "MIT License - Copyright (c) 2022 Sergey Dikiy");

		return 0;
	}

	if (logPrintNone)
	{
		logLevel = log_level::NoLogs;
	}
	else if (logPrintVerbose)
	{
		logLevel = log_level::Verbose;
	}

	if (!sourceDir.empty())
	{
		if (!std::filesystem::is_directory(sourceDir))
		{
			CF_LOG(Error, "Not a directory: {0}", sourceDir.generic_string());
			return ret_code::SourceDirNotValid;
		}
	}
	else
	{
		sourceDir = std::filesystem::current_path();
	}

	// if clang-format exec path was provided. Try find it there.
	// otherwise we would try to find clang-format by common paths
	if (!formatExecPath.empty())
	{
		// in case path was relative, convert to absolute.
		// we want to work with files in abosulute paths
		formatExecPath = std::filesystem::absolute(formatExecPath);
		if (!std::filesystem::is_regular_file(formatExecPath))
		{
			CF_LOG(Error, "Not a file: {0}", formatExecPath.generic_string());
			return ret_code::ClangExecArgNotValid;
		}
	}
	else
	{
#if _WIN32
		const char* exeExtension = ".exe";
#else
		const char* exeExtension = "";
#endif

		formatExecPath = std::filesystem::absolute(std::filesystem::path(std::string(llvm) + "/bin/clang-format" + exeExtension));
		if (!std::filesystem::is_regular_file(formatExecPath))
		{
			CF_LOG(Verbose, "Looking for clang-format: {0} - failed", formatExecPath.generic_string());

			formatExecPath = std::filesystem::absolute(std::filesystem::path(std::string("clang-format") + exeExtension));
			if (!std::filesystem::is_regular_file(formatExecPath))
			{
				CF_LOG(Verbose, "Looking for clang-format: {0} - failed", formatExecPath.generic_string());

				CF_LOG(Error, "Clang-format executable not found, could not proceed.");
				return ret_code::ClangExecNotFound;
			}
		}
	}

	CF_LOG(Display, "Using clang-format: {0}", formatExecPath.generic_string());

	CF_LOG(Display, "Check clang-format version");
	const auto testFormatExecCommandline = '"' + formatExecPath.generic_string() + '"' + " --version";
	if (const int result = std::system(testFormatExecCommandline.data()); result != 0)
	{
		CF_LOG(Error, "Attempt to call clang-format resulted in return code: {0}", result);
		return ret_code::ClangExecNotFound;
	}

	CF_LOG(Display, "Formatting directory: {0}", sourceDir.generic_string());

	// log any non-valid ignore paths
	for (auto& igPath : ignorePaths)
	{
		igPath = sourceDir.generic_string() + "/" + igPath.generic_string();
		igPath = std::filesystem::absolute(igPath);
		if (!std::filesystem::exists(igPath))
		{
			CF_LOG(Error, "Ignore path DOESN'T exist: {0}", igPath.generic_string());
		}
		else
		{
			CF_LOG(Display, "Ignore path: {0}", igPath.generic_string());
		}
	}

	CF_LOG(Display, "Looking for formattable files . . .");

	// create async job of collecting formatable filepaths
	// while job is doing stuff, fancy print amount of collected files
	auto filesFuture = std::async(collectFilepaths, sourceDir, ignorePaths);
	while (1)
	{
		std::cout << "Files to format: " << totalFilesToFormat << "\r" << std::flush;

		auto status = filesFuture.wait_for(std::chrono::milliseconds(16));
		if (status == std::future_status::ready)
			break;
	}
	std::cout << "Files to format: " << totalFilesToFormat << std::endl;

	const auto [filesToFormat, ignoredFiles] = filesFuture.get();

	// only for very-verbose logging, print all files that would be ignored and format
	if (logLevel == log_level::Verbose)
	{
		CF_LOG(Verbose, "\n\n######## IGNORED FILES ########");
		for (const auto& file : ignoredFiles)
		{
			CF_LOG(Verbose, "#  {0} - ignored", file.generic_string());
		}
		CF_LOG(Verbose, "######## IGNORED FILES END ########\n\n");

		CF_LOG(Verbose, "\n\n######## FILES WAITNING FORMATTING ########");
		for (const auto& file : filesToFormat)
		{
			CF_LOG(Verbose, "#  {0} - waiting format", file.generic_string());
		}
		CF_LOG(Verbose, "######## FILES WAITNING FORMATTING END ########\n\n");
	}

	CF_LOG(Display, "Starting formatting . . .");

	// create async job of formatting files collected from previos job
	// while job formatting files, print amount of files formatted by job
	auto formatFuture = std::async(formatFiles, filesToFormat);
	while (1)
	{
		std::cout << "Formatted files: " << currentFiles << " / " << totalFilesToFormat << "\r" << std::flush;

		auto status = formatFuture.wait_for(std::chrono::milliseconds(16));
		if (status == std::future_status::ready)
			break;
	}
	std::cout << "Formatted files: " << currentFiles << " / " << totalFilesToFormat << std::endl;

	const int futureExitCode = formatFuture.get();
	CF_LOG(Display, "Exit code: {0}", futureExitCode);
	return futureExitCode;
}

void handleAbort(int sig)
{
	abortJob = true;
	CF_LOG(Error, "\nABORT RECEIVED\n");
}

void parseArgs(int argc, char* argv[])
{
	val_ref const* prev_arg = nullptr;
	for (int i = 0; i < argc; ++i)
	{
		const std::string_view arg = argv[i];

		// find if argument listed in args
		const auto found_arg = std::find_if(std::begin(args), std::end(args), [&arg](const val_ref& val)
			{ return arg == val.name; });

		// some arguments doesn't need options and some does
		// we sort it out by separation bool and non-bool arguments
		// those arguments that are bool, would be just set true (no options required)
		// for others on next iteration, next value of arg (unless it is a argument) would be their option
		// option would be parsed into one of the possible types of prev_arg
		// for options that are array type, parsing will stop when new argument found or end of list

		if (found_arg != std::end(args))
		{
			prev_arg = &(*found_arg);
			if (auto val = prev_arg->to<bool>())
			{
				*val = true;
				prev_arg = nullptr;
			}
		}
		else if (prev_arg)
		{
			if (auto val = prev_arg->to<std::string>())
			{
				*val = arg;
				prev_arg = nullptr;
			}
			else if (auto val = prev_arg->to<std::filesystem::path>())
			{
				*val = arg;
				prev_arg = nullptr;
			}
			else if (auto val = prev_arg->to<std::vector<std::filesystem::path>>())
			{
				val->push_back(arg);
			}
			else
			{
				CF_LOG(Error, "Failed parse argument: {0}. Type not supported: {1}", prev_arg->name, prev_arg->type.name());
			}
		}
		else
		{
			if (arg.ends_with("clang-format-all") || arg.ends_with("clang-format-all.exe"))
				continue;

			CF_LOG(Error, "Unknown argument: {0}", arg.data());
		}
	}
}

void parseEnvp(char* envp[])
{
	for (int i = 0; envp[i] != NULL; ++i)
	{
		const std::string_view env = envp[i];
		const std::string_view env_name = env.substr(0, env.find_first_of('='));

		// find environment variables that we might need
		const auto found_env = std::find_if(std::begin(env_vars), std::end(env_vars), [&env_name](const val_ref& val)
			{ return val.name == env_name; });

		// if found_env variable found
		// separate env variable from it's contents
		// check if found_env are supported type
		// save contents to found_env
		if (found_env != std::end(env_vars))
		{
			const std::string_view env_value = env.substr(env.find_first_of('=') + 1);
			if (auto val = found_env->to<std::string_view>())
			{
				*val = env_value;
			}
			else
			{
				CF_LOG(Error, "Failed parse environment variable: {0}. Type not supported: {1}", found_env->name, found_env->type.name());
			}
		}
	}
}

pathsArrayTuple collectFilepaths(const std::filesystem::path& dir, const std::vector<std::filesystem::path>& ignorePaths)
{
	std::vector<std::filesystem::path> filesToFormat{};
	filesToFormat.reserve(4096);
	std::vector<std::filesystem::path> filesToIgnore{};
	filesToIgnore.reserve(4096);

	// go through all directories
	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
	{
		// if entry not a file, continue to next one
		if (!entry.is_regular_file())
			continue;

		// check if file is C/Cxx one
		const auto filePath = std::filesystem::absolute(entry.path());
		const bool isCxxExtension = std::find(extensions.begin(), extensions.end(), filePath.filename().extension().generic_string()) != extensions.end();
		if (!isCxxExtension)
			continue;

		// check if ignored paths applied to the file
		bool isIgnored = false;
		for (const auto& igPath : ignorePaths)
		{
			if (std::filesystem::is_directory(igPath))
			{
				// igDirStr -> ./include/super/cool
				// filePathStr -> ./include/super/cool/boi/path/do-not-format.h
				// ...................................^ strip here
				// and compare the remaining string with igDirStr

				const auto igDirStr = igPath.generic_string();
				auto filePathStr = filePath.generic_string();
				if (const auto pos = filePathStr.find_first_of('/', igDirStr.size() - 1); pos != std::string::npos)
					filePathStr.erase(pos);

				isIgnored = filePathStr == igDirStr;
			}
			else if (std::filesystem::is_regular_file(igPath) && igPath == filePath)
			{
				isIgnored = true;
			}

			if (isIgnored)
			{
				break;
			}
		}

		if (isIgnored)
		{
			filesToIgnore.push_back(filePath);
			continue;
		}

		// we collected a file to run clang-format on!
		filesToFormat.push_back(filePath);
		totalFilesToFormat = filesToFormat.size();
	}

	return pathsArrayTuple{filesToFormat, filesToIgnore};
}

int formatFiles(const std::vector<std::filesystem::path>& files)
{
	std::atomic<int> result = ret_code::Ok;

	const auto lambda = [&result](const std::filesystem::path& path)
	{
		if (!abortJob)
		{
			// -i is nessesery command for clang-format to run on a file
			// we allow use of additional commandline arguments that
			const auto baseCommand = std::string('"' + formatExecPath.generic_string() + '"' + " -i " + path.generic_string());
			const auto fullCommand = baseCommand + " " + formatCommands;
			const int commandRes = std::system(fullCommand.data());
			// if executed command returned non-zero value, it means something went wrong with it.
			if (commandRes != 0)
			{
				result = ret_code::ClangFormatError;
			}
			++currentFiles;
		}
	};

	// go through all provided filepaths in parallel and execute lambda on each one
	if (!abortJob)
		std::for_each(std::execution::par_unseq, files.begin(), files.end(), lambda);

	return result;
}