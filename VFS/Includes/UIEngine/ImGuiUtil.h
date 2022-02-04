// Author : Jihong Shin (snowapril)

#if !defined(UI_ENGINE_IMGUI_UTIL_H)
#define UI_ENGINE_IMGUI_UTIL_H

struct ImGuiStyle;

namespace ImGui
{
	void StyleCinder(ImGuiStyle* dst);
	bool FileOpen(const char* label, const char* btn, char* buf, size_t buf_size, const char* title, int filter_num,
				  const char* const* filter_patterns);
}

#endif