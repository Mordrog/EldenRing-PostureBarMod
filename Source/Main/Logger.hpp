#pragma once
#include "../Common.hpp"

namespace ER 
{
	enum class LogLevel
	{
		Debug,
		Log,
		Warning,
		Error
	};


	class Logger 
	{
	public:
		static inline const char* logFileName = "mods\\PostureModLog.txt";
		static inline std::ofstream logFile;

		explicit Logger() = delete;
		~Logger() noexcept = delete;
		Logger(Logger const&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger const&) = delete;
		Logger& operator=(Logger&&) = delete;
		
		static void log(const std::string_view& msg, LogLevel logLevel = LogLevel::Log, const std::source_location& location = std::source_location::current());
		static void close();

		static inline bool useLogger = false;
		static inline std::mutex loggerMutex;
	};
}