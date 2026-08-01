#include "OutputLog/OutputLogWidget.h"

#include "Logging/LogBus.h"

namespace AE::Editor
{
namespace
{
const char* LevelName(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Info:
		return "Info";
	case LogLevel::Warning:
		return "Warning";
	case LogLevel::Error:
		return "Error";
	}

	return "Unknown";
}

ImVec4 LevelColor(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Info:
		return ImVec4(0.78f, 0.82f, 0.86f, 1.0f);
	case LogLevel::Warning:
		return ImVec4(1.0f, 0.72f, 0.22f, 1.0f);
	case LogLevel::Error:
		return ImVec4(1.0f, 0.34f, 0.30f, 1.0f);
	}

	return ImVec4(0.78f, 0.82f, 0.86f, 1.0f);
}

bool IsLevelVisible(LogLevel level, bool showInfo, bool showWarnings, bool showErrors)
{
	switch (level)
	{
	case LogLevel::Info:
		return showInfo;
	case LogLevel::Warning:
		return showWarnings;
	case LogLevel::Error:
		return showErrors;
	}

	return true;
}
} // namespace

void OutputLogWidget::Draw(bool* open)
{
	if (open && !*open)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(620.0f, 260.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Output Log", open))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button("Clear"))
	{
		LogBus::Clear();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &autoScroll);
	ImGui::SameLine();
	ImGui::Checkbox("Info", &showInfo);
	ImGui::SameLine();
	ImGui::Checkbox("Warnings", &showWarnings);
	ImGui::SameLine();
	ImGui::Checkbox("Errors", &showErrors);

	filter.Draw("Filter", -1.0f);

	const std::vector<LogBusEntry> entries = LogBus::GetEntriesSnapshot();
	ImGui::Text("Entries: %zu / %zu", entries.size(), LogBus::GetMaxEntries());
	ImGui::Separator();

	ImGui::BeginChild("OutputLogScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
	const bool shouldScrollToBottom = autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

	for (const LogBusEntry& entry : entries)
	{
		if (!IsLevelVisible(entry.level, showInfo, showWarnings, showErrors))
		{
			continue;
		}

		if (!filter.PassFilter(entry.message.c_str()) && !filter.PassFilter(entry.category.c_str()))
		{
			continue;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, LevelColor(entry.level));
		ImGui::TextUnformatted(LevelName(entry.level));
		ImGui::PopStyleColor();

		ImGui::SameLine(82.0f);
		ImGui::TextDisabled("[%s]", entry.category.c_str());
		ImGui::SameLine(170.0f);
		ImGui::TextUnformatted(entry.message.c_str());
	}

	if (shouldScrollToBottom)
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::EndChild();
	ImGui::End();
}

} // namespace AE::Editor
