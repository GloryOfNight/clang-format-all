#pragma once
#include "log.hxx"

#include <array>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

struct val_ref // helper struct to improve workflow for command line arguments
{
	template <typename T>
	constexpr val_ref(const std::string_view& inName, T& inValue, const std::string_view& inNoteHelp)
		: name{inName}
		, note_help{inNoteHelp}
		, value{&inValue}
		, type{typeid(T)}
	{
	}

	std::string_view name;
	std::string_view note_help;
	void* value;
	const std::type_info& type;

	template <typename T>
	T* to() const
	{
		return type == typeid(T) ? reinterpret_cast<T*>(value) : nullptr;
	}
};

static bool printHelp;									 // when true - prints help and exits
static bool logPrintNone;								 // when true - disable all logs
static bool logPrintVerbose;							 // when true - enable verbose logging (could slow performance)
static std::filesystem::path sourceDir{};				 // path to source directory to format
static std::filesystem::path formatExecPath{};			 // clang-format executable path
static std::string formatCommands{};					 // additional clang-format arguments to run
static std::vector<std::filesystem::path> ignorePaths{}; // paths to ignore within sourceDir path
// clang-format off
static constexpr auto args = std::array
{
	val_ref{"--help", printHelp,			"--help                         = print help" },
	val_ref{"--no-logs", logPrintNone,		"--no-logs                      = disable logs (might improve performance slightly)" },
	val_ref{"--verbose", logPrintVerbose,	"--verbose                      = enable verbose logs" },
	val_ref{"-S", sourceDir,				"-S <path-to-source>            = directory to format" },
	val_ref{"-E", formatExecPath,			"-E <path-to-clang-format>      = clang-format executable path" },
	val_ref{"-I", ignorePaths,				"-I <dirs-list>                 = list of paths to ignore relative to -S" },
	val_ref{"-C", formatCommands,			"-C <args-clang-format>         = additional command-line arguments for clang-format executable" }
};
// clang-format on

static std::string_view llvm;
// clang-format off
static constexpr auto env_vars = std::array
{
	val_ref{"LLVM", llvm, ""}
};
// clang-format on

static constexpr std::array<std::string_view, 6> extensions = {".cpp", ".cxx", ".c", ".h", ".hxx", ".hpp"};

static std::atomic<bool> abortJob = false;	 // either job thread should be aborted or not
static std::atomic<size_t> totalFiles = 0;	 // total amount of files to run clang-format on
static std::atomic<size_t> currentFiles = 0; // current amount of files that was formatted by clang-format