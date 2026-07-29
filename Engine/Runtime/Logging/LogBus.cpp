#include "Logging/LogBus.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace AE
{
namespace
{
constexpr SizeT DefaultMaxEntries = 2000u;

std::vector<LogBusEntry>& Entries()
{
	static std::vector<LogBusEntry> entries;
	return entries;
}

std::mutex& EntriesMutex()
{
	static std::mutex mutex;
	return mutex;
}

SizeT& MaxEntries()
{
	static SizeT maxEntries = DefaultMaxEntries;
	return maxEntries;
}

uint64& NextSequence()
{
	static uint64 nextSequence = 1u;
	return nextSequence;
}

void TrimOldEntries(std::vector<LogBusEntry>& entries)
{
	const SizeT maxEntries = MaxEntries();
	if (entries.size() <= maxEntries)
	{
		return;
	}

	entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(entries.size() - maxEntries));
}
} // namespace

void LogBus::Add(LogLevel level, std::string category, std::string message)
{
	std::lock_guard<std::mutex> lock(EntriesMutex());

	LogBusEntry entry;
	entry.sequence = NextSequence()++;
	entry.level = level;
	entry.category = std::move(category);
	entry.message = std::move(message);

	std::vector<LogBusEntry>& entries = Entries();
	entries.push_back(std::move(entry));
	TrimOldEntries(entries);
}

std::vector<LogBusEntry> LogBus::GetEntriesSnapshot()
{
	std::lock_guard<std::mutex> lock(EntriesMutex());
	return Entries();
}

void LogBus::Clear()
{
	std::lock_guard<std::mutex> lock(EntriesMutex());
	Entries().clear();
}

void LogBus::SetMaxEntries(SizeT maxEntries)
{
	std::lock_guard<std::mutex> lock(EntriesMutex());
	MaxEntries() = std::max<SizeT>(1u, maxEntries);
	TrimOldEntries(Entries());
}

SizeT LogBus::GetMaxEntries()
{
	std::lock_guard<std::mutex> lock(EntriesMutex());
	return MaxEntries();
}

} // namespace AE
