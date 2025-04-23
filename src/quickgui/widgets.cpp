#include "quickgui/widgets.hpp"
#include "quickgui/sdl.hpp"

namespace quickgui {
namespace widgets {

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

inline const ImColor green = ImColor::HSV(2.f/7.f, 0.6f, 0.6f);
inline const ImColor green_hovered = ImColor::HSV(2.f/7.0f, 0.7f, 0.7f);
inline const ImColor green_active = ImColor::HSV(2.f/7.0f, 0.8f, 0.8f);


bool checkbox_negated(const char *title, bool *var)
{
    bool not_var = !*var;
    if(ImGui::Checkbox(title, &not_var))
    {
        *var = !not_var;
        return true;
    }
    return false;
}


void push_disabled()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.3f);
}

void pop_disabled()
{
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

void push_disabled(bool disabled)
{
    if(disabled)
        push_disabled();
}

void pop_disabled(bool disabled)
{
    if(disabled)
        pop_disabled();
}

void push_enabled(bool enabled)
{
    if(!enabled)
        push_disabled();
}

void pop_enabled(bool enabled)
{
    if(!enabled)
        pop_disabled();
}


// Make the UI compact
void push_compact()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, (float)(int)(style.FramePadding.y * 0.70f)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, (float)(int)(style.ItemSpacing.y * 0.70f)));
}

void pop_compact()
{
    ImGui::PopStyleVar(2);
}

bool toggle_button(const char *btn_txt, bool *var)
{
    if(*var)
    {
        ImGui::PushID(var);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)green);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)green_hovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)green_active);
        ImGui::Button(btn_txt);
        if(ImGui::IsItemClicked(0))
            *var = false;
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    else
    {
        if(ImGui::Button(btn_txt))
            *var = true;
    }
    return *var;
}

void set_hover_tooltip(const char *tooltip)
{
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tooltip);
}
void set_tooltip(const char *tooltip)
{
    set_hover_tooltip(tooltip);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

//-----------------------------------------------------------------------------

bool compact_color_picker(const char *title, ucolor *color, const char *tooltip)
{
    fcolor f = *color;
    bool ret = compact_color_picker(title, &f, tooltip);
    if(ret)
        *color = ucolor(f);
    return ret;
}

bool compact_color_picker(const char *title, fcolor *color, const char *tooltip)
{
    ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;
    bool ret = ImGui::ColorEdit4(title, &color->r, flags);
    if(tooltip)
        set_tooltip(tooltip);
    return ret;
}


//-----------------------------------------------------------------------------

ImRect get_current_plot_rect()
{
    ImRect r;
    r.Min = ImPlot::GetPlotPos();
    ImVec2 wp = ImGui::GetWindowPos();
    r.Min.x += 7.5f;
    r.Min.x -= wp.x;
    r.Min.y -= wp.y;
    r.Max = ImPlot::GetPlotSize();
    r.Max.x += r.Min.x;
    r.Max.y += r.Min.y;
    return r;
}


//-----------------------------------------------------------------------------

void set_column_contents_aligned_right(int col, c4::csubstr txt)
{
    ImGui::TableSetColumnIndex(col);
    set_column_contents_aligned_right(txt);
}

float get_column_padding_for_aligned_right_contents(c4::csubstr txt)
{
    /* https://stackoverflow.com/questions/58044749/how-to-right-align-text-in-imgui-columns */
    const float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(txt.str).x
                        - ImGui::GetScrollX() - ImGui::GetStyle().ItemSpacing.x);
    return posX;
}

void set_column_padding_for_aligned_right_contents(c4::csubstr txt)
{
    /* https://stackoverflow.com/questions/58044749/how-to-right-align-text-in-imgui-columns */
    const float posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(txt.str).x
                        - ImGui::GetScrollX() - ImGui::GetStyle().ItemSpacing.x);
    if(posX > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(posX);
}


void set_column_contents_aligned_right(c4::csubstr txt)
{
    set_column_padding_for_aligned_right_contents(txt);
    ImGui::TextUnformatted(txt.str);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void vulkan_window(WidgetState *st)
{
    if(!st->show_vulkan_window)
        return;
    ImGui::Begin("Vulkan", &st->show_vulkan_window);
    bool debug = st->debug_vulkan;
    ImGui::Checkbox("Enable debug", &debug);
    if(debug != st->debug_vulkan)
    {
        st->debug_vulkan = debug;
        rhi::enable_vk_debug(debug);
    }
    ImGui::End();
}

inline constexpr const ImGuiWindowFlags bg_flags = ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoScrollbar
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoNav
    | ImGuiWindowFlags_NoNavInputs
    | ImGuiWindowFlags_NoBackground
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    | ImGuiWindowFlags_NoSavedSettings;

void draw_bg(quickgui::GuiAssets::Image const& img, ImVec2 topleft, ImVec2 pivot)
{
    ImGui::PushID(&img);
    ImVec2 excess = {30.f, 30.f};
    ImVec2 img_size = img.size();
    ImVec2 window_size = {img_size.x + excess.x, img_size.y + excess.y};
    ImGui::SetNextWindowPos(topleft, ImGuiCond_Always, pivot);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
    ImGui::Begin("bg", nullptr, bg_flags);
    img.display();
    ImGui::End();
    ImGui::PopID();
}


void demo(WidgetState *st)
{
    if(!st->show_demo_window)
        return;
    ImGui::Begin("Demos", &st->show_demo_window);
    ImGui::Text("%.1f ms/frame (%.1f FPS)",
                (double)(1000.f / ImGui::GetIO().Framerate),
                (double)ImGui::GetIO().Framerate);
    ImGui::Checkbox("Vulkan Widget", &st->show_vulkan_window);
    #ifdef QUICKGUI_WITH_DEMOS
    ImGui::Checkbox("Imgui demo Widget", &st->show_demo_widget);
    ImGui::Checkbox("Implot demo Widget", &st->show_plot_demo_widget);
    #endif
    ImGuiIO& io = ImGui::GetIO();
    const float MIN_SCALE = 0.3f;
    const float MAX_SCALE = 2.0f;
    ImGui::PushItemWidth(ImGui::GetFontSize() * 8);
    ImGui::DragFloat("Font scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();
    ImGui::ColorEdit3("Clear color", (float*)&st->clear_color);
    #ifdef QUICKGUI_WITH_DEMOS
    if(st->show_demo_widget)
        ImGui::ShowDemoWindow(&st->show_demo_widget);
    if(st->show_plot_demo_widget)
        ImPlot::ShowDemoWindow(&st->show_plot_demo_widget);
    #endif
    ImGui::End();
}

void events(WidgetState *st)
{
    quickgui::gui_iter_events([](SDL_Event const& event, void *data){
        auto *wst = (WidgetState*)data;
        #if SDL_MAJOR_VERSION == 3
        if(event.type == SDL_EVENT_KEY_DOWN)
        {
            #define _TOGGLE_ON_CTRL(key, var) SDLK_##key: { if(SDL_GetModState() & (SDL_KMOD_LCTRL|SDL_KMOD_RCTRL)) { (var) = !(var); return true; } } break
        #else
        if(event.type == SDL_KEYDOWN)
        {
            #define _TOGGLE_ON_CTRL(key, var) SDLK_##key: { if(SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL)) { (var) = !(var); return true; } } break
        #endif
            switch(event.key.keysym.sym)
            {
            case _TOGGLE_ON_CTRL(d, wst->show_demo_window);
            case _TOGGLE_ON_CTRL(v, wst->show_vulkan_window);
            }
            #undef _TOGGLE_ON_CTRL
        }
        return false;
    }, st);
}

} // namespace widgets



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void DynamicImage::set_name(const char *name)
{
    rhi_img.set_name(&rhi::g_rhi, name);
}

void DynamicImage::reset(uint32_t width, uint32_t height, VkFormat format)
{
    rhi::ImageLayout layout = {};
    layout.width = width;
    layout.height = height;
    layout.format = format;
    rhi_img.reset(&rhi::g_rhi, layout.to_vk());
    flip_count = 0;
    for(size_t i : irange(RhiImage::num_entries))
        gui_img[i].load_existing(rhi_img.img_ids[i], g_gui_assets.default_sampler);
}

void DynamicImage::flip()
{
    rhi_img.flip();
    ++flip_count;
}

C4_SUPPRESS_WARNING_GCC_CLANG_POP

} // namespace quickgui
