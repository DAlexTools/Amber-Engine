#include "Common/LegacyDiagnosticsOverlay.h"

#include "Logging/LogBus.h"
#include "Renderer/SDL/Graphics.h"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr Uint32 PanelColor = 0x101014D8;
constexpr Uint32 PanelBorderColor = 0x5A6C7AFF;
constexpr Uint32 TextColor = 0xE8EEF5FF;
constexpr Uint32 MutedTextColor = 0xA8B4C0FF;
constexpr Uint32 WarningTextColor = 0xFFC857FF;
constexpr Uint32 ErrorTextColor = 0xFF5C5CFF;

std::string FormatMs(const char* label, double value)
{
	std::ostringstream stream;
	stream.precision(3);
	stream << std::fixed << label << ": " << value << " ms";
	return stream.str();
}

std::string FormatFps(double fps)
{
	std::ostringstream stream;
	stream.precision(1);
	stream << std::fixed << "FPS: " << fps;
	return stream.str();
}

std::string Truncate(const std::string& value, SizeT maxChars)
{
	if (value.size() <= maxChars)
	{
		return value;
	}

	if (maxChars <= 3u)
	{
		return value.substr(0u, maxChars);
	}

	return value.substr(0u, maxChars - 3u) + "...";
}

void DrawPanel(int x, int y, int width, int height)
{
	boxColor(Graphics::Renderer, x, y, x + width, y + height, PanelColor);
	rectangleColor(Graphics::Renderer, x, y, x + width, y + height, PanelBorderColor);
}

void DrawTextLine(int x, int& y, const std::string& text, Uint32 color = TextColor)
{
	stringColor(Graphics::Renderer, static_cast<Sint16>(x), static_cast<Sint16>(y), text.c_str(), color);
	y += 12;
}

Uint32 ColorForLogLevel(AE::LogLevel level)
{
	switch (level)
	{
	case AE::LogLevel::Warning:
		return WarningTextColor;
	case AE::LogLevel::Error:
		return ErrorTextColor;
	case AE::LogLevel::Info:
	default:
		return TextColor;
	}
}

const char* NameForLogLevel(AE::LogLevel level)
{
	switch (level)
	{
	case AE::LogLevel::Warning:
		return "WARN";
	case AE::LogLevel::Error:
		return "ERR ";
	case AE::LogLevel::Info:
	default:
		return "INFO";
	}
}
} // namespace

void LegacyDiagnosticsOverlay::BeginFrame()
{
	const uint64 now = Counter();
	if (previousFrameCounter != 0u)
	{
		const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
		frameMs = (static_cast<double>(now - previousFrameCounter) * 1000.0) / frequency;
		fps = frameMs > 0.0 ? 1000.0 / frameMs : 0.0;
	}
	previousFrameCounter = now;
}

void LegacyDiagnosticsOverlay::HandleEvent(const SDL_Event& event)
{
	if (event.type != SDL_KEYDOWN || event.key.repeat != 0)
	{
		return;
	}

	switch (event.key.keysym.sym)
	{
	case SDLK_F1:
		showPerformance = !showPerformance;
		break;
	case SDLK_F2:
		showControls = !showControls;
		break;
	case SDLK_F3:
		showOutputLog = !showOutputLog;
		break;
	case SDLK_F4:
		AE::LogBus::Clear();
		break;
	case SDLK_p:
		paused = !paused;
		break;
	default:
		break;
	}
}

void LegacyDiagnosticsOverlay::SetUpdateMs(double ms)
{
	updateMs = ms;
}

void LegacyDiagnosticsOverlay::SetRenderMs(double ms)
{
	renderMs = ms;
}

void LegacyDiagnosticsOverlay::Draw(const LegacyDiagnosticsData& data) const
{
	if (!Graphics::Renderer)
	{
		return;
	}

	if (showPerformance)
	{
		const bool hasWorld = data.world != nullptr;
		const int panelHeight = hasWorld ? 230 : 118;
		DrawPanel(12, 12, 360, panelHeight);

		int y = 22;
		DrawTextLine(22, y, std::string("Performance - ") + (data.sampleName ? data.sampleName : "Sample"));
		DrawTextLine(22, y, FormatFps(fps));
		DrawTextLine(22, y, FormatMs("Frame", frameMs));
		DrawTextLine(22, y, FormatMs("Update", updateMs));
		DrawTextLine(22, y, FormatMs("Render", renderMs));
		DrawTextLine(22, y, std::string("Paused: ") + (paused ? "yes" : "no"), paused ? WarningTextColor : TextColor);
		DrawTextLine(22, y, std::string("Debug draw: ") + (data.debugDraw ? "on" : "off"));

		if (data.particleCount > 0u)
		{
			DrawTextLine(22, y, "Particles: " + std::to_string(data.particleCount));
		}

		if (data.stickCount > 0u)
		{
			DrawTextLine(22, y, "Sticks: " + std::to_string(data.stickCount));
		}

		if (hasWorld)
		{
			const AE::Physics::WorldStats& stats = data.world->GetLastStats();
			DrawTextLine(22, y, "Bodies: " + std::to_string(data.world->GetBodies().size()));
			DrawTextLine(22, y, "Contacts: " + std::to_string(data.world->GetContacts().size()));
			DrawTextLine(22, y, "Constraints: " + std::to_string(data.world->GetConstraints().size()));
			DrawTextLine(22, y, "Pairs: " + std::to_string(stats.bruteForcePairs) + " -> " + std::to_string(stats.broadPhasePairs));
			DrawTextLine(22, y, "Narrow tests: " + std::to_string(stats.narrowPhaseTests));
			DrawTextLine(22, y, FormatMs("Physics", stats.totalStepMs));
			DrawTextLine(22, y, FormatMs("Broad", stats.broadPhaseMs));
			DrawTextLine(22, y, FormatMs("Narrow", stats.narrowPhaseMs));
			DrawTextLine(22, y, FormatMs("Solver", stats.solverPhaseMs));
			DrawTextLine(
				22,
				y,
				"Islands: " + std::to_string(stats.solverIslandCount) + " largest " + std::to_string(stats.largestSolverIslandBodyCount) + "/" + std::to_string(stats.largestSolverIslandConstraintCount));
			DrawTextLine(
				22,
				y,
				"Parallel N/S: " + std::string(stats.parallelNarrowPhaseUsed ? "on" : "off") + "/" + (stats.parallelSolverUsed ? "on" : "off"));
			DrawTextLine(
				22,
				y,
				"Jobs N/S: " + std::to_string(stats.parallelNarrowPhaseJobs) + "/" + std::to_string(stats.parallelSolverJobs));
		}
	}

	if (showControls)
	{
		const int panelWidth = 520;
		const int panelX = std::max(12, Graphics::Width() - panelWidth - 12);
		DrawPanel(panelX, 12, panelWidth, 88);
		int y = 22;
		DrawTextLine(panelX + 10, y, "Controls");
		DrawTextLine(panelX + 10, y, "F1 performance  F2 controls  F3 output log  F4 clear log  P pause", MutedTextColor);
		if (data.controls && data.controls[0] != '\0')
		{
			DrawTextLine(panelX + 10, y, Truncate(data.controls, 62u), MutedTextColor);
		}
	}

	if (showOutputLog)
	{
		const int panelWidth = std::max(360, std::min(760, Graphics::Width() - 24));
		const int panelHeight = 150;
		const int panelX = 12;
		const int panelY = std::max(12, Graphics::Height() - panelHeight - 12);
		DrawPanel(panelX, panelY, panelWidth, panelHeight);

		int y = panelY + 10;
		const std::vector<AE::LogBusEntry> entries = AE::LogBus::GetEntriesSnapshot();
		DrawTextLine(panelX + 10, y, "Output Log: " + std::to_string(entries.size()) + " / " + std::to_string(AE::LogBus::GetMaxEntries()));
		const SizeT visibleLines = 9u;
		const SizeT firstEntry = entries.size() > visibleLines ? entries.size() - visibleLines : 0u;
		const SizeT maxTextChars = static_cast<SizeT>(std::max(20, (panelWidth - 24) / 8));

		for (SizeT i = firstEntry; i < entries.size(); ++i)
		{
			const AE::LogBusEntry& entry = entries[i];
			std::string line = std::string(NameForLogLevel(entry.level)) + " [" + entry.category + "] " + entry.message;
			DrawTextLine(panelX + 10, y, Truncate(line, maxTextChars), ColorForLogLevel(entry.level));
		}
	}
}

bool LegacyDiagnosticsOverlay::IsPaused() const
{
	return paused;
}

bool& LegacyDiagnosticsOverlay::ShowPerformance()
{
	return showPerformance;
}

bool& LegacyDiagnosticsOverlay::ShowControls()
{
	return showControls;
}

bool& LegacyDiagnosticsOverlay::ShowOutputLog()
{
	return showOutputLog;
}

bool& LegacyDiagnosticsOverlay::Paused()
{
	return paused;
}

uint64 LegacyDiagnosticsOverlay::Counter()
{
	return SDL_GetPerformanceCounter();
}

double LegacyDiagnosticsOverlay::ElapsedMilliseconds(uint64 startCounter)
{
	const uint64 endCounter = Counter();
	const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
	return (static_cast<double>(endCounter - startCounter) * 1000.0) / frequency;
}
