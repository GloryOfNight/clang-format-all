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

struct val_ref
{
	template <typename T>
	constexpr val_ref(const std::string_view& inName, const std::string_view& inNote, T& inValue)
		: name{inName}
		, note{inNote}
		, value{&inValue}
		, type{typeid(T)}
	{
	}

	std::string_view name;
	std::string_view note;
	void* value;
	const std::type_info& type;

	template <typename T>
	T* to() const
	{
		return type == typeid(T) ? reinterpret_cast<T*>(value) : nullptr;
	}
};

static bool printHelp;
static bool logUseDisabled;
static bool logUseVerbose;
static std::filesystem::path sourceDir;
static std::filesystem::path formatExecPath;
static std::string formatCommands;
static std::vector<std::filesystem::path> ignorePaths;
// clang-format off
static constexpr auto args = std::array
{
	val_ref{"--help", "print help", printHelp},
	val_ref{"--no-logs", "disable logs (might improve performance slightly)", logUseDisabled},
	val_ref{"--verbose", "enable verbose logs", logUseVerbose},
	val_ref{"-S", "source directory to format", sourceDir},
	val_ref{"-E", "clang-format executable path", formatExecPath},
	val_ref{"-I", "space separated list of paths to ignore relative to [-S]", formatCommands},
	val_ref{"-C", "additional command-line arguments for clang-format executable", ignorePaths}
};
// clang-format on

static std::string_view llvm;
// clang-format off
static constexpr auto env_vars = std::array
{
	val_ref{"LLVM","", llvm}
};
// clang-format on

static constexpr std::array<std::string_view, 6> extensions = {".cpp", ".cxx", ".c", ".h", ".hxx", ".hpp"};

static std::atomic<bool> abortJob = false;
static std::atomic<size_t> totalFiles = 0;
static std::atomic<size_t> currentFiles = 0;

enum // exit codes
{
	RET_OK = 0,
	RET_CLANG_FORMAT_EXEC_NOT_FOUND = 1,
	RET_ARG_SOURCE_NOT_VALID = 2,
	RET_ARG_EXEC_NOT_VALID = 3,
	RET_CLANG_FORMAT_ERROR = 4
};

enum // log levels
{
	DISABLED = -1,
	VERBOSE = 0,
	DISPLAY = 1,
	ERROR = 2
};
// current log level as well as default
static int logLevel = DISPLAY;

void handleAbort(int sig);

void parseArgs(int argc, char* argv[]);
void parseEnvp(char* envp[]);

void log(int level, const char* log, ...);
int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir, std::vector<std::filesystem::path>&& ignoreDirs);

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

	log(DISPLAY, "Starting job thread. . .");

	auto future = std::async(doJob, formatExecPath, sourceDir, std::move(ignorePaths));
	if (logLevel >= VERBOSE)
	{
		while (!abortJob)
		{
			std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << "\r" << std::flush;
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
		}
		std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << std::endl;
	}
	future.wait();

	const int futureExitCode = future.get();
	log(DISPLAY, "Exit code: %i", futureExitCode);
	return futureExitCode;
}

void handleAbort(int sig)
{
	abortJob = true;
	log(ERROR, "\nABORT RECEIVED\n");
}

void log(int level, const char* log, ...)
{
	if (logLevel <= DISABLED)
		return;

	if (level < logLevel)
		return;

	va_list list;
	va_start(list, log);
	switch (level)
	{
	case VERBOSE:
		std::vfprintf(stdout, (std::string(log) + '\n').data(), list);
		break;
	case DISPLAY:
		std::vfprintf(stdout, (std::string(log) + '\n').data(), list);
		break;
	case ERROR:
		std::vfprintf(stderr, (std::string(log) + '\n').data(), list);
		break;
	default:;
	}
	va_end(list);
}

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir, std::vector<std::filesystem::path>&& ignorePaths)
{
	std::vector<std::filesystem::path> files;
	files.reserve(4096);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
	{
		if (abortJob)
			break;

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
	}
	totalFiles = files.size();

	std::atomic<int> result = RET_OK;

	const auto lambda = [&formatExecutable, &result](const std::filesystem::path& path)
	{
		if (!abortJob)
		{
			const auto baseCommand = std::string('"' + formatExecutable.generic_string() + '"' + " -i " + path.generic_string());
			const auto fullCommand = baseCommand + formatCommands;
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

	abortJob = true;
	return result;
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
