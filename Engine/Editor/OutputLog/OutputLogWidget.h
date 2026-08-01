#ifndef ENGINE_EDITOR_OUTPUT_LOG_WIDGET_H
#define ENGINE_EDITOR_OUTPUT_LOG_WIDGET_H

#include "imgui.h"

namespace AE::Editor
{

class OutputLogWidget
{
public:
	void Draw(bool* open = nullptr);

private:
	ImGuiTextFilter filter;
	bool autoScroll = true;
	bool showInfo = true;
	bool showWarnings = true;
	bool showErrors = true;
};

} // namespace AE::Editor

#endif
