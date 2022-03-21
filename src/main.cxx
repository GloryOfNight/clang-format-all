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

enum
{
	OK = 0,
	CLANG_FORMAT_EXE_NOT_FOUND = 1,
	SRC_DIR_NOT_FOUND = 2
} error_codes;

enum
{
	DISPLAY = 0,
	ERROR = 1
} log_level;

void handleAbort(int sig);

void log(int level, const char* log, ...);

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir);

int main(int argc, char* argv[])
{
	std::signal(SIGABRT, handleAbort);
	std::signal(SIGINT, handleAbort);
	std::signal(SIGTERM, handleAbort);

	const char* LLVMdir = std::getenv("LLVM");
	if (LLVMdir == nullptr)
		LLVMdir = "";

#if _WIN32
	const char* exeStem = ".exe";
#else
	const char* exeStem = "";
#endif
	const auto clangFormatExecutable = std::filesystem::path(LLVMdir + std::string("/bin/clang-format") + exeStem);
	if (!std::filesystem::exists(clangFormatExecutable) && std::filesystem::is_regular_file(clangFormatExecutable))
	{
		log(ERROR, "Clang-format executable not found, could not proceed.");
		return CLANG_FORMAT_EXE_NOT_FOUND;
	}
	log(DISPLAY, "Using clang-format: %s", clangFormatExecutable.generic_string().data());

	auto thread = std::thread(doJob, clangFormatExecutable, std::filesystem::current_path());

	while (!abortJob)
	{
		const size_t copyTotalFiles = totalFiles;
		const size_t copyCurrentFiles = currentFiles;
		std::cout << "Formatted files: " << copyCurrentFiles << " / " << copyTotalFiles << "\r" << std::flush;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	std::cout << "Formatted files: " << currentFiles << " / " << totalFiles << std::endl;

	thread.join();
	return OK;
}

void handleAbort(int sig)
{
	abortJob = true;
	log(ERROR, "\nABORT RECEIVED\n");
}

void log(int level, const char* log, ...)
{
	const std::string logNewLine = std::string(log) + '\n';

	va_list list;
	va_start(list, log);
	switch (level)
	{
	case DISPLAY:
		std::vfprintf(stdout, logNewLine.data(), list);
		break;
	case ERROR:
		std::vfprintf(stderr, logNewLine.data(), list);
		break;
	default:;
	}
	va_end(list);
}

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir)
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
		if (std::find(extensions.begin(), extensions.end(), filePath.filename().extension().generic_string()) != extensions.end())
		{
			files.push_back(filePath);
		}
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
		std::for_each(std::execution::par, files.begin(), files.end(), lambda);

	abortJob = true;
	return OK;
}
