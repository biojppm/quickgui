#ifndef VIDEO_PLAYER_GUI_HPP_
#define VIDEO_PLAYER_GUI_HPP_

#include <quickgui/widgets.hpp>
#include <quickgui/log.hpp>
#include <quickgui/time.hpp>
#include <quickgui/mem.hpp>
#include <quickgui/video/video_frame.hpp>
#include "./video_reader.hpp"

struct VideoPlayerGui
{
    static inline constexpr const uint32_t num_entries = quickgui::DynamicImage::RhiImage::num_entries;

    QuickguiVideoReader m_reader;
    bool                m_first_render;
    bool                m_playing;
    bool                m_load_paused_frame;
    bool                m_reader_frame_ready;
    bool                m_show_video_window;
    quickgui::DynamicImage      m_img;
    quickgui::GuiAssets::Image  m_gui[num_entries];
    bool                m_gui_flipped;
    bool                m_gui_display_on;
    int                 m_gui_display_size;
    int                 m_gui_display_size_item;

    VideoPlayerGui(quickgui::VideoSource const& src)
        : m_reader(src, true)
        , m_first_render(true)
        , m_playing(false)
        , m_load_paused_frame(false)
        , m_reader_frame_ready(false)
        , m_show_video_window(false)
        , m_img()
        , m_gui()
        , m_gui_flipped(false)
        , m_gui_display_on(true)
        , m_gui_display_size(2048)
        , m_gui_display_size_item(4)
    {
        //VkFormat fmt = VK_FORMAT_R8G8B8_UNORM;
        //VkFormat fmt = VK_FORMAT_R8_UNORM;//imgview_format(m_reader.data_type(), m_reader.num_channels());
        //VkFormat fmt = imgview_format(m_reader.data_type(), m_reader.num_channels());
        //VkFormat fmt = imgview_format(m_reader.data_type(), 3);
        VkFormat fmt = VK_FORMAT_R8G8B8A8_UNORM;
        m_img.reset(m_reader.width(), m_reader.height(), fmt);
        for(uint32_t i : quickgui::irange(num_entries))
        {
            m_img.set_name("videoplayer");
            m_gui[i].load_existing(m_img.rhi_img.img_ids[i], quickgui::g_gui_assets.default_sampler);
        }
        m_show_video_window = true;
    }

    ImVec2 dims() const { return ImVec2{(float)m_reader.width(), (float)m_reader.height()}; }

    bool finished() const { return m_reader.finished(); }
    void skip_frame()
    {
        C4_CHECK(m_reader.check_frame_ready());
        m_reader.read_frame();
    }

    std::chrono::nanoseconds time_curr() const
    {
        return m_reader.time_curr();
    }

    uint32_t frame_curr() const
    {
        return m_reader.frame_curr();
    }

    bool update(quickgui::VideoFrame *fr)
    {
        if((!m_playing) && (!m_load_paused_frame))
            return false;
        auto ret = _update(fr);
        if(!m_load_paused_frame)
        {
            _upload_to_curr();
        }
        else
        {
            _upload_to_all();
            m_load_paused_frame = false;
        }
        return ret;
    }

    bool _update(quickgui::VideoFrame *fr)
    {
        bool ready = m_reader.check_frame_ready();
        if(ready)
        {
            m_reader.read_frame();
            fr->index = frame_curr();
            fr->timestamp = time_curr();
            m_reader.vflip_frame();
        }
        return ready;
    }

    void _upload_to_all()
    {
        for(uint32_t _ : quickgui::irange(num_entries))
        {
            C4_UNUSED(_);
            _upload_to_curr();
        }
    }

    void _upload_to_curr()
    {
        charspan mem = {}; // the memory for rendering
        mem = m_img.rhi_img.start_wcpu();
        m_reader.convert_channels();
        m_reader.copy_to_display_buffer(mem);
        m_img.rhi_img.finish_wcpu(mem);
        m_img.flip();
    }

    struct Commands
    {
        bool cmd_toggle_video, cmd_prev_video_frame, cmd_next_video_frame;
    };

    /** @p draw_glyphs_fn a function to overlay glyphs on top of the
     * rendered video */
    template<class DrawFn>
    void render(Commands *cmds, DrawFn &&draw_glyphs_fn)
    {
        if(m_first_render)
        {
            _upload_to_all();
            m_first_render = false;
        }
        if(!m_show_video_window)
            return;
        ImGui::Begin("Video", &m_show_video_window);
        auto &r = m_img.rhi_img;
        auto &g = m_gui[r.rgpu];
        // widget: flip video render
        ImGui::Checkbox("v-flip video render", &m_gui_flipped);
        quickgui::widgets::set_tooltip("vertically flip the rendered image");
        // widget: video
        _render_video_size_widget();
        ImVec2 dims = {(float)m_gui_display_size, (float)m_gui_display_size};
        // flip vertically. This is purely a render-flip.
        if(m_gui_flipped)
        {
            g.uv_botr.y = 0.f;
            g.uv_topl.y = 1.f;
        }
        else
        {
            g.uv_botr.y = 1.f;
            g.uv_topl.y = 0.f;
        }
        if(ImGui::Button(m_playing ? "PAUSE" : "PLAY") || cmds->cmd_toggle_video)
        {
            m_playing = !m_playing;
            cmds->cmd_toggle_video = false;
        }
        quickgui::widgets::set_hover_tooltip("play/pause (Ctrl-Space)");
        {
            auto s = quickgui::widgets::DisabledScope(m_playing);
            uint32_t fr = m_reader.frame_curr();
            ImGui::SameLine();
            {
                auto s_ = quickgui::widgets::DisabledScope(fr < 2);
                if(ImGui::Button("PREV") || (!s_.disabled && cmds->cmd_prev_video_frame))
                {
                    m_reader.frame_curr(fr - 2);
                    m_load_paused_frame = true;
                    cmds->cmd_prev_video_frame = false;
                }
                quickgui::widgets::set_hover_tooltip("previous frame (Ctrl-Left)");
            }
            ImGui::SameLine();
            {
                auto s_ = quickgui::widgets::DisabledScope(fr+1 == m_reader.num_frames());
                if(ImGui::Button("NEXT") || (!s_.disabled && cmds->cmd_next_video_frame))
                {
                    //m_reader.frame_curr(fr + 1);
                    m_load_paused_frame = true;
                    cmds->cmd_next_video_frame = false;
                }
                quickgui::widgets::set_hover_tooltip("next frame (Ctrl-Right)");
            }
        }
        ImGui::Text("Frame %u %fms\n", frame_curr(), (double)quickgui::fmsecs(time_curr()).count());
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        if(m_gui_display_on)
        {
            g.display(dims);
        }
        draw_glyphs_fn(cursor, dims, ImGui::GetWindowDrawList());
        ImGui::End();
    }

    void _render_video_size_widget()
    {
        const char* combo_items[] = { "original", "256", "512", "1024", "2048", "custom" };
        auto combo_display_size = [&](int item){
            switch(item)
            {
            case 0: return (int)m_reader.width();
            case 1: return 256;
            case 2: return 512;
            case 3: return 1024;
            case 4: return 2048;
            case 5: return m_gui_display_size;
            }
            return 512;
        };
        m_gui_display_size = combo_display_size(m_gui_display_size_item);
        auto combo_display_item = [&](int sz){
            switch(sz)
            {
            case 256: return 1;
            case 512: return 2;
            case 1024: return 3;
            case 2048: return 4;
            default:
                if(sz == (int)m_reader.width())
                    return 0;
                else if(sz == m_gui_display_size)
                    return 5;
            }
            return 3;
        };
        int currsz = m_gui_display_size;
        ImGui::Checkbox("render video", &m_gui_display_on);
        {
            auto ds = quickgui::widgets::DisabledScope(!m_gui_display_on);
            const char *tooltip = "in-window size of the displayed video";
            ImGui::Text("Video Display Size:");
            ImGui::SetNextItemWidth(120.f);
            if(ImGui::Combo("preset", &m_gui_display_size_item, combo_items, IM_ARRAYSIZE(combo_items)))
            {
                m_gui_display_size = combo_display_size(m_gui_display_size_item);
            }
            quickgui::widgets::set_hover_tooltip(tooltip);
            ImGui::SetNextItemWidth(120.f);
            ImGui::SameLine();
            if(ImGui::InputInt("custom", &currsz))
            {
                m_gui_display_size = currsz;
                m_gui_display_size_item = combo_display_item(currsz);
            }
            quickgui::widgets::set_tooltip(tooltip);
        }
    }
};


#endif /* VIDEO_PLAYER_GUI_HPP_ */
