#pragma once
#include "statics.hxx"
#include "types.hxx"

#include <cstdarg>
#include <format>
#include <iostream>

void log(const log_level level, const char* log, ...)
{
	if (level <= log_level::NoLogs)
		return;

	if (level < logLevel)
		return;

	va_list list;
	va_start(list, log);
	switch (level)
	{
	case log_level::Verbose:
		std::vfprintf(stdout, (std::string(log) + '\n').data(), list);
		break;
	case log_level::Display:
		std::vfprintf(stdout, (std::string(log) + '\n').data(), list);
		break;
	case log_level::Error:
		std::vfprintf(stderr, (std::string(log) + '\n').data(), list);
		break;
	default:;
	}
	va_end(list);
}

#define CF_LOG(level, format, ...) log(log_level::level, format, ##__VA_ARGS__);