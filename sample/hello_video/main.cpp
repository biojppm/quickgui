#include <quickgui/widgets.hpp>
#include <quickgui/video/video_source.hpp>
#include <quickgui/overlay_canvas.hpp>
#include <quickgui/sdl.hpp>
#include "./sample_video_player.hpp"

void draw_hello_world();
void add_and_draw_primitives(quickgui::PrimitiveDrawList *primitives, ImVec2 video_size);
void handle_video_shortcuts(SampleVideoPlayer::Commands *cmds);

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

int main()
{
    quickgui::widgets::WidgetState st;
    quickgui::GuiConfig gcfg = {};
    if (!quickgui::gui_init(gcfg))
        return -1;

    quickgui::VideoFrame frame = {};
    quickgui::VideoSource video_source = {};
    video_source.source_type = quickgui::VideoSource::CAMERA;
    video_source.camera.index = 0;
    SampleVideoPlayer video_player(video_source);
    SampleVideoPlayer::Commands video_player_commands;

    quickgui::PrimitiveDrawList primitives = {};
    quickgui::OverlayCanvas overlay_canvas = {};
    auto draw_glyphs = [&video_player, &primitives, &overlay_canvas]
        (ImVec2 canvas_pos, ImVec2 canvas_dims, ImDrawList *draw_list){
        overlay_canvas.draw(primitives, video_player.dims(), canvas_pos, canvas_dims, draw_list);
    };

    while (true)
    {
        if(!quickgui::gui_start_frame())
            break;
        // do work here
        if(video_player.update(&frame))
        {
            // process frame.
            // as an example, we only create primitives to overlay.
            // in a real application, these primitives could be
            // markings from feature extraction
        }
        // draw the gui
        quickgui::widgets::events(&st);
        handle_video_shortcuts(&video_player_commands);
        quickgui::widgets::draw_bg();
        add_and_draw_primitives(&primitives, video_player.dims());
        video_player.render(&video_player_commands, draw_glyphs);
        quickgui::widgets::vulkan_window(&st);
        #ifdef QUICKGUI_WITH_DEMOS
        quickgui::widgets::demo(&st);
        #endif
        quickgui::gui_end_frame(st.clear_color);
    }
    quickgui::gui_terminate();
    return 0;
}

void draw_hello_world()
{
    static bool show_hello_world = false;
    ImGui::Begin("Hello");
    if(ImGui::Button("HELLO!"))
        show_hello_world = true;
    quickgui::widgets::set_hover_tooltip("click me!");
    ImGui::Text("Press Ctrl-D for debug window");
    ImGui::End();
    if(show_hello_world)
    {
        ImGui::Begin("Hello World", &show_hello_world);
        ImGui::Text("HELLO WORLD!");
        if(ImGui::Button("bye."))
            show_hello_world = false;
        quickgui::widgets::set_hover_tooltip("close this window");
        ImGui::End();
    }
}

template<class Fn>
void _draw_primitive_widgets(const char *primitive_name,
                             bool *enabled, quickgui::ucolor *color, float *thickness,
                             Fn draw_param_widgets)
{
    if(ImGui::TreeNode(primitive_name))
    {
        ImGui::Checkbox("draw", enabled);
        {
            auto ds = quickgui::widgets::DisabledScope(!*enabled);
            ImGui::SameLine();
            quickgui::widgets::compact_color_picker("color", color);
            ImGui::SliderFloat("thickness", thickness, 0.5f, 20.f);
            draw_param_widgets();
        }
        ImGui::TreePop();
    }
}

// draw an example of each type of primitive, and offer widgets to
// tweak the primitive's parameters.
void add_and_draw_primitives(quickgui::PrimitiveDrawList *primitives, ImVec2 video_size)
{
    static bool show_primitives = true;
    primitives->clear();
    if(ImGui::Begin("Primitives", &show_primitives))
    {
        using quickgui::ucolor;
        auto draw_widget_pos = [video_size](const char *name, ImVec2 *pos){
            ImGui::SliderFloat2(name, (float*)pos, 0.f, maxof(video_size));
        };
        // text
        {
            static bool draw_text = true;
            static ImVec2 text_pos = ImVec2{0.33f, 0.33f} * video_size;
            static ucolor text_color = quickgui::colors::yellow_pure;
            static const char text[] = "wow this is nice";
            static float text_thickness = 1.f;
            _draw_primitive_widgets("text", &draw_text, &text_color, &text_thickness, [&]{
                draw_widget_pos("pos", &text_pos);
            });
            if(draw_text)
                primitives->draw_text(text, text_pos, text_color, text_thickness);
        }
        // point
        {
            static bool draw_point = true;
            static ImVec2 point_pos = ImVec2{0.44f, 0.44f} * video_size;
            static ucolor point_color = quickgui::colors::yellowish;
            static float point_thickness = 1.f;
            _draw_primitive_widgets("point", &draw_point, &point_color, &point_thickness, [&]{
                draw_widget_pos("pos", &point_pos);
            });
            if(draw_point)
                primitives->draw_point(point_pos, point_color, point_thickness);
        }
        // line
        {
            static bool draw_line = true;
            static ImVec2 line_pt0 = ImVec2{0.11f, 0.22f} * video_size;
            static ImVec2 line_pt1 = ImVec2{0.22f, 0.11f} * video_size;
            static ucolor line_color = quickgui::colors::red;
            static float line_thickness = 1.f;
            _draw_primitive_widgets("line", &draw_line, &line_color, &line_thickness, [&]{
                draw_widget_pos("line pt0", &line_pt0);
                draw_widget_pos("line pt1", &line_pt1);
            });
            if(draw_line)
                primitives->draw_line(line_pt0, line_pt1, line_color, line_thickness);
        }
        // rect
        {
            static bool draw_rect = true;
            static ImVec2 rect_pt0 = ImVec2{0.11f, 0.33f} * video_size;
            static ImVec2 rect_pt1 = ImVec2{0.22f, 0.55f} * video_size;
            static ucolor rect_color = quickgui::colors::blue;
            static float rect_thickness = 1.f;
            _draw_primitive_widgets("rect", &draw_rect, &rect_color, &rect_thickness, [&]{
                draw_widget_pos("rect pt0", &rect_pt0);
                draw_widget_pos("rect pt1", &rect_pt1);
            });
            if(draw_rect)
                primitives->draw_rect(ImRect{rect_pt0, rect_pt1}, rect_color, rect_thickness);
        }
        // rect filled
        {
            static bool draw_rect_filled = true;
            static ImVec2 rect_filled_pt0 = ImVec2{0.14f, 0.37f} * video_size;
            static ImVec2 rect_filled_pt1 = ImVec2{0.18f, 0.45f} * video_size;
            static ucolor rect_filled_color = alpha(quickgui::colors::reddish, (uint8_t)(0x5f));
            static float rect_filled_thickness = 1.f;
            _draw_primitive_widgets("rect_filled", &draw_rect_filled, &rect_filled_color, &rect_filled_thickness, [&]{
                draw_widget_pos("rect_filled pt0", &rect_filled_pt0);
                draw_widget_pos("rect_filled pt1", &rect_filled_pt1);
            });
            if(draw_rect_filled)
                primitives->draw_rect_filled(ImRect{rect_filled_pt0, rect_filled_pt1}, rect_filled_color, rect_filled_thickness);
        }
        // circle
        {
            static bool draw_circle = true;
            static ImVec2 circle_center = ImVec2{0.7f, 0.37f} * video_size;
            static float circle_radius = 0.1f * minof(video_size);
            static ucolor circle_color = quickgui::colors::reddish;
            static float circle_thickness = 1.f;
            _draw_primitive_widgets("circle", &draw_circle, &circle_color, &circle_thickness, [&]{
                draw_widget_pos("circle center", &circle_center);
                ImGui::SliderFloat("radius", &circle_radius, 0.f, maxof(video_size));
            });
            if(draw_circle)
                primitives->draw_circle(circle_center, circle_radius, circle_color, circle_thickness);
        }
    }
    ImGui::End();
}

void handle_video_shortcuts(SampleVideoPlayer::Commands *cmds)
{
    quickgui::gui_iter_events([](SDL_Event const& event, void *cmds_v){
        auto *cmds_ = (SampleVideoPlayer::Commands*)cmds_v;
        if(event.type == SDL_KEYDOWN)
        {
            #define _TOGGLE_ON_CTRL(key, var) SDLK_##key: { if(SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL)) { (var) = !(var); return true; } } break
            switch(event.key.keysym.sym)
            {
            case _TOGGLE_ON_CTRL(RIGHT, cmds_->cmd_next_video_frame);
            case _TOGGLE_ON_CTRL(LEFT, cmds_->cmd_prev_video_frame);
            case _TOGGLE_ON_CTRL(SPACE, cmds_->cmd_toggle_video);
            }
            #undef _TOGGLE_ON_CTRL
        }
        return false;
    }, cmds);
}

C4_SUPPRESS_WARNING_GCC_CLANG_POP
