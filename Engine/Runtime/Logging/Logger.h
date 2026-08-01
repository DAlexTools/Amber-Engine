#ifndef ENGINE_RUNTIME_LOGGER_H
#define ENGINE_RUNTIME_LOGGER_H
#include <vector>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#ifdef _EXPORTING
#define ENGINE_API __declspec(dllexport)
#elif defined(_IMPORTING)
#define ENGINE_API __declspec(dllimport)
#else
#define ENGINE_API
#endif

#elif defined(__linux__) || defined(__APPLE__)
#ifdef _EXPORTING
#define ENGINE_API __attribute__((visibility("default")))
#else
#define ENGINE_API
#endif

#else
#error "Unsupported platform for ENGINE_API"
#endif

/**
 * Enum representing the type of log entry.
 */
namespace AE
{

enum ENGINE_API LogType
{
	LOG_INFO,	 /**< Informational log entry */
	LOG_WARNING, /**< Warning log entry */
	LOG_ERROR	 /**< Error log entry */
};

/**
 * Struct representing a single log entry with a type and message.
 */
struct ENGINE_API LogEntry
{
	LogType type;		 /**< Type of log entry */
	std::string message; /**< Message content of the log entry */
};

/**
 * Logger class responsible for managing and outputting log messages.
 */
class ENGINE_API Logger
{
public:
	/**
	 * Static list of all log entries recorded.
	 */
	ENGINE_API static std::vector<LogEntry> messages;

	/**
	 * Logs an informational message.
	 *
	 * @param message The message to log as an informational entry.
	 */
	ENGINE_API static void Log(const std::string& message, const std::string& category = "Runtime");

	/**
	 * Logs a warning message to the console in yellow color.
	 *
	 * @param message The warning message to log.
	 */
	ENGINE_API static void Warn(const std::string& message, const std::string& category = "Runtime");

	/**
	 * Logs an error message.
	 *
	 * @param message The message to log as an error entry.
	 */
	ENGINE_API static void Err(const std::string& message, const std::string& category = "Runtime");

	ENGINE_API static void Warning(const std::string& message, const std::string& category = "Runtime");
	ENGINE_API static void Error(const std::string& message, const std::string& category = "Runtime");

	/**
	 * Static function from save log file in project  folder
	 */
	ENGINE_API static void SaveLogToFile();
	ENGINE_API static void SetConsoleEnabled(bool enabled);

private:
	ENGINE_API static std::string filePath;
	ENGINE_API static bool consoleEnabled;
};

} // namespace AE

#endif
