// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/Logger.h>
#include <cassert>

namespace vfs
{
	enum class LogLevel : unsigned char
	{
		INFO_LEVEL = 0,
		LOG_LEVEL = 1,
		ERROR_LEVEL = 2,
	};

	Logger::~Logger()
	{
		destroyLogger();
	}

	bool Logger::initialize(const char* logFileName, LogLevel logLevel)
	{
		unsigned char level = static_cast<unsigned char>(logLevel);

		// TODO(snowapril) : connect cout, clog, cerr to given file stream
		if (level <= static_cast<unsigned char>(LogLevel::INFO_LEVEL))
		{
			
		}
		if (level <= static_cast<unsigned char>(LogLevel::LOG_LEVEL))
		{

		}
		if (level <= static_cast<unsigned char>(LogLevel::ERROR_LEVEL))
		{

		}

		return true;
	}

	void Logger::destroyLogger()
	{
	}
}