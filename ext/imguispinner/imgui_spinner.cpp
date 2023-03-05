#include "imgui_spinner.h"

#include <math.h>

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "imgui_internal.h"


namespace ImGui {


bool Spinner(const char* label)
{
    const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    return ImGui::Spinner(label, 15.f, 6.f, col);
}


// https://github.com/ocornut/imgui/issues/1901
bool Spinner(const char* label, float radius, float thickness, const ImU32& color)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    // Render
    window->DrawList->PathClear();

    int num_segments = 30;
    int start = (int)fabs(ImSin((float)g.Time*1.8f)*(float)(num_segments-5));

    const float a_min = IM_PI*2.0f * ((float)start) / (float)num_segments;
    const float a_max = IM_PI*2.0f * ((float)num_segments-3) / (float)num_segments;

    const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);

    for (int i = 0; i < num_segments; i++) {
        const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
        const float apt8 = a+(float)g.Time*8.f;
        window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(apt8) * radius,
                                            centre.y + ImSin(apt8) * radius));
    }

    window->DrawList->PathStroke(color, false, thickness);

    return true;
}

} // namespace ImGui

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
