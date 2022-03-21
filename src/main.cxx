#include <array>
#include <cstdarg>
#include <execution>
#include <filesystem>
#include <string>
#include <vector>

enum
{
	OK = 0,
	LLVM_NOT_FOUND = 1,
	CLANG_FORMAT_EXE_NOT_FOUND = 2,
	SRC_DIR_NOT_FOUND = 3
} error_codes;

enum
{
	VERBOSE = 0,
	WARNING = 1,
	ERROR = 2
} log_level;

void log(int level, const char* log, ...);

int doJob(const std::filesystem::path& formatExecutable, const std::filesystem::path& dir);

int main(int argc, char* argv[])
{
	const auto LLVMdir = std::filesystem::path(std::getenv("LLVM"));
	if (!std::filesystem::exists(LLVMdir))
	{
		log(ERROR, "LLVM Environment variable not found, could not proceed.");
		return LLVM_NOT_FOUND;
	}

#if _WIN32
	const char* exeStem = ".exe";
#else
	const char* exeStem = "";
#endif
	const auto clangFormatExecutable = std::filesystem::path(LLVMdir.generic_string() + "/bin/clang-format" + exeStem);
	if (!std::filesystem::exists(clangFormatExecutable))
	{
		log(ERROR, "Clang-format executable not found, could not proceed.");
		return CLANG_FORMAT_EXE_NOT_FOUND;
	}
	log(VERBOSE, "Using clang-format: %s", clangFormatExecutable.generic_string().data());

	return doJob(clangFormatExecutable, std::filesystem::current_path());
}

void log(int level, const char* log, ...)
{
	const std::string logNewLine = std::string(log) + '\n';

	va_list list;
	va_start(list, log);
	switch (level)
	{
	case VERBOSE:
		std::vfprintf(stdout, logNewLine.data(), list);
		break;
	case WARNING:
		std::vfprintf(stderr, logNewLine.data(), list);
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
		if (!entry.is_regular_file())
		{
			continue;
		}

		const auto filePath = entry.path();
		if (std::find(extensions.begin(), extensions.end(), filePath.filename().extension().generic_string()) != extensions.end())
		{
			files.push_back(filePath);
		}
	}

	const auto lambda = [&formatExecutable](const std::filesystem::path& path)
	{
		const auto command = std::string('"' + formatExecutable.generic_string() + '"' + " -i " + path.generic_string());
		const int res = std::system(command.data());
		if (res == OK)
			log(VERBOSE, "Format complete: %s", path.generic_string().data());
		else
			log(ERROR, "Format failed with code: %i", res);
	};

	std::for_each(std::execution::par, files.begin(), files.end(), lambda);

	return OK;
}
