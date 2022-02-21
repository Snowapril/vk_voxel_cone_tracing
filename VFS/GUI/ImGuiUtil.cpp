// Author : Jihong Shin (snowapril)

#include <pch.h>
#include <Common/Utils.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include <GUI/ImGuiUtil.h>
#include <map>

namespace ImGui
{
	void StyleCinder(ImGuiStyle* dst) {
		ImGuiStyle& style = dst ? *dst : ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.ScrollbarRounding = 0.0f;
		style.TabRounding = 0.0f;
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 0.50f);
		colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

		colors[ImGuiCol_Separator] = colors[ImGuiCol_MenuBarBg];
		colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_Separator];
		colors[ImGuiCol_SeparatorHovered] = colors[ImGuiCol_Separator];

		colors[ImGuiCol_Tab] = colors[ImGuiCol_Button];
		colors[ImGuiCol_TabHovered] = colors[ImGuiCol_ButtonHovered];
		colors[ImGuiCol_TabActive] = colors[ImGuiCol_ButtonActive];
		colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
	}

	bool FileOpen(const char* label, const char* btn, char* buf, size_t buf_size, const char* title, int filter_num,
		const char* const* filter_patterns) {
		bool ret = ImGui::InputText(label, buf, buf_size);
		ImGui::SameLine();

		if (ImGui::Button(btn)) {
			const char* filename = tinyfd_openFileDialog(title, "", filter_num, filter_patterns, nullptr, false);
			if (filename)
				strcpy(buf, filename);
			ret = true;
		}
		return ret;
	}

	struct PlotVarData
	{
		ImGuiID        ID;
		std::vector<float>  Data;
		int            DataInsertIdx;
		int            LastFrame;

		PlotVarData() : ID(0), DataInsertIdx(0), LastFrame(-1) {}
	};

	typedef std::map<ImGuiID, PlotVarData> PlotVarsMap;
	static PlotVarsMap	g_PlotVarsMap;

	// Plot value over time
	// Call with 'value == FLT_MAX' to draw without adding new value to the buffer
	void ImGui::PlotVar(const char* label, float value, float scale_min, float scale_max, size_t buffer_size)
	{
		assert(label);
		if (buffer_size == 0)
			buffer_size = 120;

		ImGui::PushID(label);
		ImGuiID id = ImGui::GetID("");

		// Lookup O(log N)
		PlotVarData& pvd = g_PlotVarsMap[id];

		// Setup
		if (pvd.Data.capacity() != buffer_size)
		{
			pvd.Data.resize(buffer_size);
			memset(&pvd.Data[0], 0, sizeof(float) * buffer_size);
			pvd.DataInsertIdx = 0;
			pvd.LastFrame = -1;
		}

		// Insert (avoid unnecessary modulo operator)
		if (static_cast<size_t>(pvd.DataInsertIdx) == buffer_size)
			pvd.DataInsertIdx = 0;
		int display_idx = pvd.DataInsertIdx;
		if (value != FLT_MAX)
			pvd.Data[pvd.DataInsertIdx++] = value;

		// Draw
		int current_frame = ImGui::GetFrameCount();
		if (pvd.LastFrame != current_frame)
		{
			ImGui::Text("%s\t%-3.4f(ms)", label, pvd.Data[display_idx]);	// Display last value in buffer
			ImGui::PlotLines("##plot", &pvd.Data[0], static_cast<int>(buffer_size), pvd.DataInsertIdx, NULL, scale_min, scale_max, ImVec2(0, 40));
			pvd.LastFrame = current_frame;
		}

		ImGui::PopID();
	}

	void ImGui::PlotVarFlushOldEntries()
	{
		int current_frame = ImGui::GetFrameCount();
		for (PlotVarsMap::iterator it = g_PlotVarsMap.begin(); it != g_PlotVarsMap.end(); )
		{
			PlotVarData& pvd = it->second;
			if (pvd.LastFrame < current_frame - vfs::max(400, (int)pvd.Data.size()))
				it = g_PlotVarsMap.erase(it);
			else
				++it;
		}
	}
}