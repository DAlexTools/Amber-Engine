#include "Logging/LogBus.h"
#include "Logging/Logger.h"

#include <gtest/gtest.h>

TEST(LogBusTests, CapturesRuntimeAndPhysicsLogEntries)
{
    AE::LogBus::Clear();
    AE::Logger::SetConsoleEnabled(false);

    AE::Logger::Log("runtime log bus test");
    AE::Logger::Warning("physics log bus test", "Physics");

    const std::vector<AE::LogBusEntry> entries = AE::LogBus::GetEntriesSnapshot();

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].level, AE::LogLevel::Info);
    EXPECT_EQ(entries[0].category, "Runtime");
    EXPECT_NE(entries[0].message.find("runtime log bus test"), std::string::npos);

    EXPECT_EQ(entries[1].level, AE::LogLevel::Warning);
    EXPECT_EQ(entries[1].category, "Physics");
    EXPECT_NE(entries[1].message.find("physics log bus test"), std::string::npos);
}

TEST(LogBusTests, TrimsOldEntriesToLimit)
{
    AE::LogBus::Clear();
    AE::LogBus::SetMaxEntries(2u);

    AE::LogBus::Add(AE::LogLevel::Info, "Test", "first");
    AE::LogBus::Add(AE::LogLevel::Info, "Test", "second");
    AE::LogBus::Add(AE::LogLevel::Info, "Test", "third");

    const std::vector<AE::LogBusEntry> entries = AE::LogBus::GetEntriesSnapshot();

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].message, "second");
    EXPECT_EQ(entries[1].message, "third");

    AE::LogBus::SetMaxEntries(2000u);
    AE::LogBus::Clear();
}
