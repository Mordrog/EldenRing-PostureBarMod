#include "Logger.hpp"
#include "../PostureBarMod.hpp"

namespace ER 
{
	std::string_view logLevelText(LogLevel logLevel)
	{
		switch (logLevel)
		{
			using enum LogLevel;

			case Log:
				return "Log: ";
			case Warning:
				return "Warning: ";
			case Error:
				return "Error: ";
			case Debug:
				return "Debug: ";

			default:
				return "";
		}
	}

	std::string locationText(const std::source_location& location)
	{
		std::string string = std::string(location.file_name()) + "(" + std::to_string(location.line()) + ":" + std::to_string(location.column()) + ") '" + location.function_name() + "'|\t";
		return string;
	}

	void Logger::log(const std::string_view& msg, LogLevel logLevel, const std::source_location& location)
	{
#ifndef DEBUGLOG
		if (logLevel == LogLevel::Debug)
			return;
#endif // DEBUGLOG

		if (!useLogger)
			return;

		std::scoped_lock lock(loggerMutex);
		if (!logFile.is_open())
			logFile.open(dllPath + logFileName, std::ifstream::out);

		if (logFile.good())
		{
			logFile << locationText(location) << logLevelText(logLevel) << msg << "\n";
			logFile.flush();
		}
	}

	void Logger::close()
	{
		logFile.close();
	}
}
