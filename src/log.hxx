#pragma once
#include "statics.hxx"
#include "types.hxx"

#include <cstdarg>
#include <format>
#include <iostream>

namespace cf
{
	template <typename... Args>
	void log(const log_level level, const std::string_view format, Args... args)
	{
		if (level <= log_level::NoLogs || level < logLevel)
			return;

		if (level == log_level::Verbose || level == log_level::Display)
		{
			std::cout << std::vformat(format, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
		}
		else if (level == log_level::Error)
		{
			std::cerr << std::vformat(format, std::make_format_args(std::forward<Args>(args)...)) << std::endl;
		}
	}
} // namespace cf

#define CF_LOG(level, format, ...) cf::log(log_level::level, format, ##__VA_ARGS__);