#include "exit_codes.hxx"
#include "files_collector.hxx"
#include "files_formatter.hxx"
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

void handleAbort(int sig);

void parseArgs(int argc, char* argv[]);
void parseEnvp(char* envp[]);

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
			log(DISPLAY, "\t[%s]   %s", arg.name.data(), arg.note.data());
		}

		log(DISPLAY, "\nSource code page: https://github.com/GloryOfNight/clang-format-all.git");
		log(DISPLAY, "MIT License - Copyright (c) 2022 Sergey Dikiy");

		return 0;
	}

	if (logUseDisabled)
	{
		logLevel = DISABLED;
	}
	else if (logUseVerbose)
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
		if (formatExecPath.is_relative())
		{
			formatExecPath = std::filesystem::absolute(formatExecPath);
		}

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
			log(ERROR, "Clang-format executable not found, could not proceed.");
			return RET_CLANG_FORMAT_EXEC_NOT_FOUND;
		}
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

	log(DISPLAY, "Looking for formatable files . . .");
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

		const auto res = std::find_if(std::begin(args), std::end(args), [&arg](const val_ref& val)
			{ return arg == val.name; });

		if (res != std::end(args))
		{
			prev_arg = &(*res);
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
		}
	}
}

void parseEnvp(char* envp[])
{
	for (int i = 0; envp[i] != NULL; ++i)
	{
		const std::string_view env = envp[i];
		const std::string_view env_name = env.substr(0, env.find_first_of('='));

		const auto res = std::find_if(std::begin(env_vars), std::end(env_vars), [&env_name](const val_ref& val)
			{ return val.name == env_name; });

		if (res != std::end(env_vars))
		{
			const std::string_view env_value = env.substr(env.find_first_of('=') + 1);
			if (auto val = res->to<std::string_view>())
			{
				*val = env_value;
			}
		}
	}
}
