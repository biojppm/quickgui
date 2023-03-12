#include <quickgui/widgets.hpp>
#include <quickgui/video/video_source.hpp>
#include "./video_player_gui.hpp"
#include "./video_overlay_canvas.hpp"

void draw_hello_world();

int main()
{
    quickgui::widgets::WidgetState st;
    quickgui::GuiConfig gcfg = {};
    if (!quickgui::gui_init(gcfg))
        return -1;


    VideoOverlayCanvas overlay_canvas;
    quickgui::VideoFrame frame = {};
    quickgui::VideoSource video_source = {};
    video_source.source_type = quickgui::VideoSource::CAMERA;
    video_source.camera.index = 0;
    VideoPlayerGui video_player(video_source);
    VideoPlayerGui::Commands video_player_commands;
    auto draw_glyphs = [&video_player, &overlay_canvas](ImDrawList *draw_list, ImVec2 canvas_pos, ImVec2 canvas_dims){
        overlay_canvas.draw(draw_list, video_player.dims(), canvas_pos, canvas_dims);
    };

    while (true)
    {
        if(!quickgui::gui_start_frame())
            break;
        // do work here
        if(video_player.update(&frame))
        {
            overlay_canvas.clear();
        }
        // draw the gui
        {
            quickgui::widgets::events(&st);
            quickgui::widgets::draw_bg();
            video_player.render(&video_player_commands, draw_glyphs);
            quickgui::widgets::vulkan_window(&st);
            #ifdef QUICKGUI_WITH_DEMOS
            quickgui::widgets::demo(&st);
            #endif
        }
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
