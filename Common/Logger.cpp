// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/Logger.h>
#include <iostream>
#include <ctime>
#include <mutex>

namespace vfs
{
	static std::mutex		critical;
	static LogLevel			logLevel	= LogLevel::AllLevel;
	static std::ostream*	debugStream = &std::cout;
	static std::ostream*	infoStream	= &std::cout;
	static std::ostream*	warnStream	= &std::cout;
	static std::ostream*	errorStream = &std::cerr;

	std::string LevelToString(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Debug:
			return "[Debug]";
		case LogLevel::Info:
			return "[Info]";
		case LogLevel::Warn:
			return "[Warn]";
		case LogLevel::Error:
			return "[Error]";
		case LogLevel::AllLevel:
		case LogLevel::Off:
		default:
			return "";
		}
	}

	std::ostream* LevelToStream(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Debug:
			return debugStream;
		case LogLevel::Warn:
			return warnStream;
		case LogLevel::Error:
			return errorStream;
		case LogLevel::Off:
			return nullptr;
		case LogLevel::AllLevel:
		case LogLevel::Info:
		default:
			return infoStream;
		}
	}

	Logger::Logger(LogLevel level)
		: _logLevel(level)
	{
		// Do nothing
	}

	Logger::~Logger()
	{
		std::lock_guard<std::mutex> lock(critical);
		if (static_cast<uint8_t>(_logLevel) >= static_cast<uint8_t>(logLevel))
		{
			std::ostream* stream = LevelToStream(_logLevel);
			*stream << _logBuffer.str() << std::endl;
			stream->flush();
		}
	}

	std::string	Logging::GetHeader(LogLevel level)
	{
		std::time_t now =
			std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char timeStr[20];

		// TODO(snowapril) : need to handle cross-platform time
		tm time;
		localtime_s(&time, &now);
		strftime(timeStr, sizeof(timeStr), "%F %T", &time);

		char header[256];
		snprintf(header, sizeof(header), "[%s] %s ", LevelToString(level).c_str(),
			timeStr);

		return header;
	}

	void Logging::SetDebugStream(std::ostream* stream)
	{
		std::lock_guard<std::mutex> lock(critical);
		debugStream = stream;
	}

	void Logging::SetInfoStream(std::ostream* stream)
	{
		std::lock_guard<std::mutex> lock(critical);
		infoStream = stream;
	}

	void Logging::SetWarnStream(std::ostream* stream)
	{
		std::lock_guard<std::mutex> lock(critical);
		warnStream = stream;
	}

	void Logging::SetErrorStream(std::ostream* stream)
	{
		std::lock_guard<std::mutex> lock(critical);
		errorStream = stream;
	}

	void Logging::SetAllStream(std::ostream* stream)
	{
		SetDebugStream(stream);
		SetInfoStream(stream);
		SetWarnStream(stream);
		SetErrorStream(stream);
	}

	void Logging::SetLevel(LogLevel level)
	{
		std::lock_guard<std::mutex> lock(critical);
		logLevel = level;
	}
}