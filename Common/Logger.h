// Author : Jihong Shin (snowapril)

#if !defined(COMMON_LOGGER_H)
#define COMMON_LOGGER_H

#include <sstream>
#include <string>

namespace vfs
{
	enum class LogLevel : uint8_t
	{
		AllLevel	= 0,
		Debug		= 1,
		Info		= 2,
		Warn		= 3,
		Error		= 4,
		Off			= 5,
	};

	class Logger
	{
	public:
		explicit Logger(LogLevel level);
		~Logger();
		Logger(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger& operator=(Logger&&) = delete;

	public:
		template <typename Type>
		const Logger& operator<<(const Type& x) const
		{
			_logBuffer << x;
			return *this;
		}

	private:
		LogLevel _logLevel;
		mutable std::stringstream _logBuffer;
	};

	class Logging
	{
	public:
		static std::string	GetHeader		(LogLevel level);
		static void			SetDebugStream	(std::ostream* stream);
		static void			SetInfoStream	(std::ostream* stream);
		static void			SetWarnStream	(std::ostream* stream);
		static void			SetErrorStream	(std::ostream* stream);
		static void			SetAllStream	(std::ostream* stream);
		static void			SetLevel		(LogLevel level);
	};

	extern Logger debugLogger;
	extern Logger infoLogger;
	extern Logger warnLogger;
	extern Logger errorLogger;
}

#define VFS_DEBUG \
		(vfs::Logger(vfs::LogLevel::Debug) \
		<< vfs::Logging::GetHeader(vfs::LogLevel::Debug) << "[" << __FILE__ << ":" \
		<< __LINE__ << " (" << __func__ << ")] ")

#define VFS_INFO \
		(vfs::Logger(vfs::LogLevel::Info) \
		<< vfs::Logging::GetHeader(vfs::LogLevel::Info) << "[" << __FILE__ << ":" \
		<< __LINE__ << " (" << __func__ << ")] ")

#define VFS_WARN \
		(vfs::Logger(vfs::LogLevel::Warn) \
		<< vfs::Logging::GetHeader(vfs::LogLevel::Warn) << "[" << __FILE__ << ":" \
		<< __LINE__ << " (" << __func__ << ")] ")

#define VFS_ERROR \
		(vfs::Logger(vfs::LogLevel::Error) \
		<< vfs::Logging::GetHeader(vfs::LogLevel::Error) << "[" << __FILE__ << ":" \
		<< __LINE__ << " (" << __func__ << ")] ")

#endif