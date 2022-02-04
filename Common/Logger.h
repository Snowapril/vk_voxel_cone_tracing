// Author : Jihong Shin (snowapril)

#if !defined(COMMON_LOGGER_H)
#define COMMON_LOGGER_H

#include <fstream>
#include <ctime>
#include <string>

namespace vfs
{
	enum class LogLevel : unsigned char;

	// TODO(snowapril) : support async logging system
	class Logger
	{
	public:
		explicit Logger() = default;
				~Logger();

		struct LogInfo
		{
			std::time_t time;
			LogLevel	level;
			std::string message;
		};
	public:
		bool initialize(const char* logFileName, LogLevel logLevel);
		void destroyLogger();

	private:
		LogLevel _logLevel;
	};
}

#endif