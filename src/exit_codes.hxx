#pragma once

enum // exit codes
{
	RET_OK = 0,
	RET_CLANG_FORMAT_EXEC_NOT_FOUND = 1,
	RET_ARG_SOURCE_NOT_VALID = 2,
	RET_ARG_EXEC_NOT_VALID = 3,
	RET_CLANG_FORMAT_ERROR = 4
};