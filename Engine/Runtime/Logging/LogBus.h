#ifndef ENGINE_RUNTIME_LOG_BUS_H
#define ENGINE_RUNTIME_LOG_BUS_H

#include "Core/Platform/PlatformTypes.h"

#include <string>
#include <vector>

namespace AE
{

enum class LogLevel
{
	Info,
	Warning,
	Error
};

struct LogBusEntry
{
	uint64 sequence = 0;
	LogLevel level = LogLevel::Info;
	std::string category;
	std::string message;
};

class LogBus
{
public:
	static void Add(LogLevel level, std::string category, std::string message);
	static std::vector<LogBusEntry> GetEntriesSnapshot();
	static void Clear();
	static void SetMaxEntries(SizeT maxEntries);
	static SizeT GetMaxEntries();
};

} // namespace AE

#endif
