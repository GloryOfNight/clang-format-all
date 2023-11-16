#include "exit_codes.hxx"
#include "files_utils.hxx"
#include "log.hxx"
#include "statics.hxx"

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
#include <vector>

void handleAbort(int sig); // handle abort signal from terminal or system

void parseArgs(int argc, char* argv[]); // parse argument list
void parseEnvp(char* envp[]);			// look and parse environment variables we could use

int main(int argc, char* argv[], char* envp[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	parseArgs(argc, argv);
	parseEnvp(envp);

	if (printHelp)
	{
		log(DISPLAY, "Available arguments list:");
		for (auto& arg : args)
		{
			if (arg.note_help.size() == 0)
				continue;

			log(DISPLAY, "%s", arg.note_help.data());
		}

		log(DISPLAY, "\nSource code page: https://github.com/GloryOfNight/clang-format-all.git");
		log(DISPLAY, "MIT License - Copyright (c) 2022 Sergey Dikiy");

		return 0;
	}

	if (logPrintNone)
	{
		logLevel = DISABLED;
	}
	else if (logPrintVerbose)
	{
		logLevel = VERBOSE;
	}

	if (!sourceDir.empty())
	{
		if (!std::filesystem::is_directory(sourceDir))
		{
			log(ERROR, "Not a directory: %s", sourceDir.generic_string().data());
			return RET_ARG_SOURCE_NOT_VALID;
		}
	}
	else
	{
		sourceDir = std::filesystem::current_path();
	}

	if (!formatExecPath.empty())
	{
		if (!std::filesystem::is_regular_file(formatExecPath))
		{
			log(ERROR, "Not a file: %s", formatExecPath.generic_string().data());
			return RET_ARG_EXEC_NOT_VALID;
		}
	}
	else
	{
#if _WINDOWS
		const char* exeStem = ".exe";
#else
		const char* exeStem = "";
#endif
		formatExecPath = std::filesystem::path(std::string(llvm) + "/bin/clang-format" + exeStem);
		if (!std::filesystem::is_regular_file(formatExecPath))
		{
			formatExecPath = std::filesystem::path(std::string("clang-format") + exeStem);
			if (!std::filesystem::is_regular_file(formatExecPath))
			{
				log(ERROR, "Clang-format executable not found, could not proceed.");
				return RET_CLANG_FORMAT_EXEC_NOT_FOUND;
			}
		}
	}

	if (formatExecPath.is_relative())
	{
		formatExecPath = std::filesystem::absolute(formatExecPath);
	}

	log(DISPLAY, "Using clang-format: %s", formatExecPath.generic_string().data());
	log(DISPLAY, "Formatting directory: %s", sourceDir.generic_string().data());
	for (auto& igPath : ignorePaths)
	{
		igPath = sourceDir.generic_string() + "/" + igPath.generic_string();
		if (!std::filesystem::exists(igPath))
		{
			log(ERROR, "Ignore path DO NOT exist: %s", igPath.generic_string().data());
		}
		else
		{
			log(DISPLAY, "Ignoring: %s", igPath.generic_string().data());
		}
	}

	log(DISPLAY, "Looking for formattable files . . .");
	auto filesFuture = std::async(collectFilepaths, sourceDir, std::move(ignorePaths));

	while (1)
	{
		std::cout << "Files to format: " << totalFiles << "\r" << std::flush;

		auto status = filesFuture.wait_for(std::chrono::milliseconds(1));
		if (status == std::future_status::ready)
			break;
	}
	std::cout << "Files to format: " << totalFiles << std::endl;

	log(DISPLAY, "Starting formatting . . .");
	auto formatFuture = std::async(formatFiles, std::move(filesFuture.get()));

	while (1)
	{
		std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << "\r" << std::flush;

		auto status = formatFuture.wait_for(std::chrono::milliseconds(150));
		if (status == std::future_status::ready)
			break;
	}
	std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << std::endl;

	const int futureExitCode = formatFuture.get();
	log(DISPLAY, "Exit code: %i", futureExitCode);
	return futureExitCode;
}

void handleAbort(int sig)
{
	abortJob = true;
	log(ERROR, "\nABORT RECEIVED\n");
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
				log(ERROR, "Failed parse argument: %s. Type not supported: %s", prev_arg, prev_arg->type.name());
			}
		}
		else
		{
			if (arg.ends_with("clang-format-all") || arg.ends_with("clang-format-all.exe"))
				continue;

			log(ERROR, "Unknown argument: %s", arg.data());
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
				log(ERROR, "Failed parse environment variable: %s. Type not supported: %s", found_env, found_env->type.name());
			}
		}
	}
}
