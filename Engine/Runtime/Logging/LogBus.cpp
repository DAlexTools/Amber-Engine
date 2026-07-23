#include "Logging/LogBus.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace AE
{
namespace
{
    constexpr std::size_t DefaultMaxEntries = 2000u;

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

    std::size_t& MaxEntries()
    {
        static std::size_t maxEntries = DefaultMaxEntries;
        return maxEntries;
    }

    std::uint64_t& NextSequence()
    {
        static std::uint64_t nextSequence = 1u;
        return nextSequence;
    }

    void TrimOldEntries(std::vector<LogBusEntry>& entries)
    {
        const std::size_t maxEntries = MaxEntries();
        if (entries.size() <= maxEntries)
        {
            return;
        }

        entries.erase(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(entries.size() - maxEntries));
    }
}

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

void LogBus::SetMaxEntries(std::size_t maxEntries)
{
    std::lock_guard<std::mutex> lock(EntriesMutex());
    MaxEntries() = std::max<std::size_t>(1u, maxEntries);
    TrimOldEntries(Entries());
}

std::size_t LogBus::GetMaxEntries()
{
    std::lock_guard<std::mutex> lock(EntriesMutex());
    return MaxEntries();
}

}
