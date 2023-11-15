#pragma once

enum // exit codes
{
	RET_OK = 0,
	RET_CLANG_FORMAT_EXEC_NOT_FOUND = 1, // clang-format executable couldn't be found
	RET_ARG_SOURCE_NOT_VALID = 2, // provided argument for source directory to format is not a directory or valid path
	RET_ARG_EXEC_NOT_VALID = 3, // provided argument for clang-format executable not points to executable file
	RET_CLANG_FORMAT_ERROR = 4 // encountered an error with clang-format executable
};