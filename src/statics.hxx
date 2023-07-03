#pragma once
#include "log.hxx"

#include <array>
#include <atomic>
#include <filesystem>
#include <string>
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
static std::filesystem::path sourceDir{};
static std::filesystem::path formatExecPath{};
static std::string formatCommands{};
static std::vector<std::filesystem::path> ignorePaths{};
// clang-format off
static constexpr auto args = std::array
{
	val_ref{"--help", "print help", printHelp},
	val_ref{"--no-logs", "disable logs (might improve performance slightly)", logUseDisabled},
	val_ref{"--verbose", "enable verbose logs", logUseVerbose},
	val_ref{"-S", "source directory to format", sourceDir},
	val_ref{"-E", "clang-format executable path", formatExecPath},
	val_ref{"-I", "space separated list of paths to ignore relative to [-S]", ignorePaths},
	val_ref{"-C", "additional command-line arguments for clang-format executable", formatCommands}
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