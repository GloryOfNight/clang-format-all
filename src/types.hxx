#pragma once
#include <cstdint>
#include <string_view>
#include <typeinfo>

// helper struct to improve workflow for command line arguments
struct val_ref
{
	template <typename T>
	constexpr val_ref(const std::string_view inName, T& inValue, const std::string_view inNoteHelp)
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

// return codes
enum ret_code : uint8_t
{
	Ok,					  // execution finished normally
	ClangExecNotFound,	  // clang-format executable couldn't be found
	SourceDirNotValid,	  // provided argument for source directory to format is not a directory or valid path
	ClangExecArgNotValid, // provided argument for clang-format executable not points to executable file
	ClangFormatError	  // encountered an error with clang-format executable
};

enum class log_level : uint8_t
{
	NoLogs,
	Verbose,
	Display,
	Error
};
