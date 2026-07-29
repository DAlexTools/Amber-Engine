#include "Logger.h"
#include "Core/Platform/PlatformTypes.h"
#include "Logging/LogBus.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace AE
{

ENGINE_API std::vector<LogEntry> Logger::messages;

/** log.txt file path from text file debug  */
ENGINE_API std::string Logger::filePath = "log.txt";
ENGINE_API bool Logger::consoleEnabled = true;

ENGINE_API void Logger::SaveLogToFile()
{
	std::ofstream file(filePath, std::ios::trunc);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open file for writing: " + filePath);
	}

	for (const auto& logEntry : messages)
	{
		file << logEntry.message << std::endl;
	}

	file.close();
}

/**
 * Retrieves the current date and time as a formatted string.
 *
 * @return A string representing the current date and time in the format "DD-MMM-YYYY HH:MM:SS".
 */
ENGINE_API std::string CurrentDateTimeToString()
{
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char output[32] = {};
	const SizeT written = std::strftime(output, sizeof(output), "%d-%b-%Y %H:%M:%S", std::localtime(&now));
	return std::string(output, written);
}

/**
 * Logs an informational message to the console in green color.
 *
 * @param message The informational message to log.
 */
ENGINE_API void Logger::Log(const std::string& message, const std::string& category)
{
	LogEntry logEntry;
	logEntry.type = LOG_INFO;
	logEntry.message = "LOG: [" + CurrentDateTimeToString() + "]: " + message;
	if (consoleEnabled)
	{
		std::cout << "\x1B[32m" << logEntry.message << "\033[0m" << std::endl;
	}

	messages.push_back(logEntry);
	AE::LogBus::Add(AE::LogLevel::Info, category, logEntry.message);
}

/**
 * Logs a warning message to the console in yellow color.
 *
 * @param message The warning message to log.
 */
ENGINE_API void Logger::Warn(const std::string& message, const std::string& category)
{
	LogEntry logEntry;
	logEntry.type = LOG_WARNING;
	logEntry.message = "WARN: [" + CurrentDateTimeToString() + "]: " + message;
	messages.push_back(logEntry);
	AE::LogBus::Add(AE::LogLevel::Warning, category, logEntry.message);

	if (consoleEnabled)
	{
		std::cout << "\x1B[33m" << logEntry.message << "\033[0m" << std::endl;
	}
}

/**
 * Logs an error message to the console in red color.
 *
 * @param message The error message to log.
 */
ENGINE_API void Logger::Err(const std::string& message, const std::string& category)
{
	LogEntry logEntry;
	logEntry.type = LOG_ERROR;
	logEntry.message = "ERR: [" + CurrentDateTimeToString() + "]: " + message;

	messages.push_back(logEntry);
	AE::LogBus::Add(AE::LogLevel::Error, category, logEntry.message);
	if (consoleEnabled)
	{
		std::cerr << "\x1B[91m" << logEntry.message << "\033[0m" << std::endl;
	}
}

ENGINE_API void Logger::Warning(const std::string& message, const std::string& category)
{
	Warn(message, category);
}

ENGINE_API void Logger::Error(const std::string& message, const std::string& category)
{
	Err(message, category);
}

ENGINE_API void Logger::SetConsoleEnabled(bool enabled)
{
	consoleEnabled = enabled;
}

} // namespace AE
