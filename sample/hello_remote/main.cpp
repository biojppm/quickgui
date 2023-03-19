#include <quickgui/widgets.hpp>

void draw_hello_world();

int main()
{
    quickgui::widgets::WidgetState st;
    quickgui::GuiConfig gcfg = {};
    if (!quickgui::gui_init(gcfg))
        return -1;
    while (true)
    {
        if(!quickgui::gui_start_frame())
            break;
        // do work here
        {
            // ...
        }
        // draw the gui
        {
            quickgui::widgets::events(&st);
            quickgui::widgets::draw_bg();
            draw_hello_world();
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
