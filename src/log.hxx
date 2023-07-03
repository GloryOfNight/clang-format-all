#pragma once

#include <cstdarg>
#include <iostream>


enum // log levels
{
	DISABLED = -1,
	VERBOSE = 0,
	DISPLAY = 1,
	ERROR = 2
};


// current log level as well as default
static int logLevel = DISPLAY;

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