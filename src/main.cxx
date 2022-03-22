#include <array>
#include <atomic>
#include <csignal>
#include <cstdarg>
#include <execution>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

std::atomic<bool> abortJob = false;
std::atomic<size_t> totalFiles = 0;
std::atomic<size_t> currentFiles = 0;

std::filesystem::path sourceDir;
std::filesystem::path formatExecPath;
std::vector<std::filesystem::path> ignoreDirs;

enum
{
	OK = 0,
	CLANG_FORMAT_EXEC_NOT_FOUND = 1,
	ARG_SOURCE_NOT_VALID = 2,
	ARG_EXEC_NOT_VALID = 3
} error_codes;

enum
{
	VERBOSE = 0,
	DISPLAY = 1,
	ERROR = 2
} log_levels;
int logLevel = DISPLAY;

void handleAbort(int sig);

void parseArgs(int argc, char* argv[]);

void log(int level, const char* log, ...);

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir, std::vector<std::filesystem::path>&& ignoreDirs);

int main(int argc, char* argv[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	parseArgs(argc, argv);

	if (!sourceDir.empty())
	{
		if (!std::filesystem::is_directory(sourceDir))
		{
			log(ERROR, "Not a directory: %s", sourceDir.generic_string().data());
			return ARG_SOURCE_NOT_VALID;
		}
	}
	else
	{
		sourceDir = std::filesystem::current_path();
	}

	if (!formatExecPath.empty())
	{
		if (!std::filesystem::exists(formatExecPath) && !std::filesystem::is_regular_file(formatExecPath))
		{
			log(ERROR, "Not a file: %s", formatExecPath.generic_string().data());
			return ARG_EXEC_NOT_VALID;
		}
	}
	else
	{
		const char* LLVMdir = std::getenv("LLVM");
		if (LLVMdir == nullptr)
			LLVMdir = "";
#if _WINDOWS
		const char* exeStem = ".exe";
#else
		const char* exeStem = "";
#endif
		formatExecPath = std::filesystem::path(LLVMdir + std::string("/bin/clang-format") + exeStem);
		if (!std::filesystem::exists(formatExecPath) && std::filesystem::is_regular_file(formatExecPath))
		{
			log(ERROR, "Clang-format executable not found, could not proceed.");
			return CLANG_FORMAT_EXEC_NOT_FOUND;
		}
	}
	log(DISPLAY, "Using clang-format: %s", formatExecPath.generic_string().data());
	log(DISPLAY, "Formatting directory: %s", sourceDir.generic_string().data());
	for (const auto dir : ignoreDirs)
	{
		log(DISPLAY, "Ignoring: %s", dir.generic_string().data());
	}

	log(DISPLAY, "Starting job thread. . .");
	auto thread = std::thread(doJob, formatExecPath, sourceDir, std::move(ignoreDirs));

	while (!abortJob)
	{
		std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << "\r" << std::flush;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << std::endl;

	thread.join();

	log(DISPLAY, "Done");
	return OK;
}

void handleAbort(int sig)
{
	abortJob = true;
	log(ERROR, "\nABORT RECEIVED\n");
}

void log(int level, const char* log, ...)
{
	if (level < logLevel)
		return;

	va_list list;
	va_start(list, log);
	switch (level)
	{
	case VERBOSE:
		std::vfprintf(stdout, log, list);
		std::fprintf(stderr, "\n");
		break;
	case DISPLAY:
		std::vfprintf(stdout, log, list);
		std::fprintf(stderr, "\n");
		break;
	case ERROR:
		std::vfprintf(stderr, log, list);
		std::fprintf(stderr, "\n");
		break;
	default:;
	}
	va_end(list);
}

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir, std::vector<std::filesystem::path>&& ignoreDirs)
{
	const std::array<const char*, 6> extensions = {".cpp", ".cxx", ".c", ".h", ".hxx", ".hpp"};

	std::vector<std::filesystem::path> files;
	files.reserve(1024);

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

		for (auto igDir : ignoreDirs)
		{
			const std::string igDirStr = dir.generic_string() + "/" + igDir.generic_string();
			const std::string fileRootPath = std::string(filePath.generic_string(), 0, igDirStr.size());
			if (igDirStr == fileRootPath)
			{
				isIgnored = true;
				log(VERBOSE, "Ignoring file: %s", filePath.generic_string().data());
				break;
			}
		}

		if (isIgnored)
			continue;

		files.push_back(filePath);
	}
	totalFiles = files.size();

	const auto lambda = [&formatExecutable](const std::filesystem::path& path)
	{
		if (!abortJob)
		{
			const auto command = std::string('"' + formatExecutable.generic_string() + '"' + " -i " + path.generic_string());
			const int res = std::system(command.data());
			++currentFiles;
		}
	};

	if (!abortJob)
		std::for_each(std::execution::par_unseq, files.begin(), files.end(), lambda);

	abortJob = true;
	return OK;
}

void parseArgs(int argc, char* argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		const std::string_view arg = argv[i];
		if (i + 1 < argc)
		{
			if (arg == "-S")
			{
				++i;
				sourceDir = std::filesystem::path(argv[i]);
			}
			else if (arg == "-E")
			{
				++i;
				formatExecPath = std::filesystem::path(argv[i]);
			}
			else if (arg == "-I")
			{
				++i;
				ignoreDirs.reserve(argc - i);
				for (; i < argc; ++i)
				{
					if (std::string_view(argv[i]) == "--verbose")
					{
						logLevel = VERBOSE;
						break;
					}
					ignoreDirs.push_back(std::filesystem::path(argv[i]));
				}
			}
			else if (arg == "--verbose")
			{
				logLevel = VERBOSE;
			}
		}
	}
}