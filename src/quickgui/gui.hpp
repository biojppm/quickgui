#ifndef QUICKGUI_GUI_GUI_HPP_
#define QUICKGUI_GUI_GUI_HPP_

#include "quickgui/rhi.hpp"
#include "quickgui/imgui.hpp"

// this fwd-declaration is required
typedef union SDL_Event SDL_Event;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace quickgui {

struct GuiConfig
{
    std::string window_name;
    std::string window_icon;
    uint32_t    window_width;
    uint32_t    window_height;
    uint32_t    clear_color; // input
    bool        debug_vulkan;
};


bool gui_init(GuiConfig const& cfg);
void gui_terminate();

void gui_acquire_assets();
void gui_release_assets();

bool gui_start_frame();
void gui_end_frame(ImVec4 background);

using GuiEventHandler = bool (*)(SDL_Event const& event, void *data);
void gui_iter_events(GuiEventHandler fn, void *data);

struct GuiImage
{
    rhi::image_id      img_id = {};
    rhi::image_view_id view_id = {};
    VkDescriptorSet    desc_set = {};
    ImVec2             uv_topl = {0.f, 0.f};
    ImVec2             uv_botr = {1.f, 1.f};
    ImVec4             tint_color = {1.f, 1.f, 1.f, 1.f};
    ImVec4             border_color = {0.f, 0.f, 0.f, 0.f};
    void load_existing(rhi::image_id id);
    void load_existing(rhi::image_id id, rhi::sampler_id sampler);
    void load(const char *filename, ccharspan img_data, rhi::ImageLayout const& layout);
    void load(const char *filename, ccharspan img_data, rhi::ImageLayout const& layout, rhi::sampler_id sampler, VkCommandBuffer cmd_buf);
    void load(const char *filename);
    void load(const char *filename, rhi::sampler_id sampler, VkCommandBuffer cmd_buf);
    void destroy();
    rhi::ImageLayout const& layout() const { return rhi::g_rhi.get_image(img_id).layout; }
    void display(float scale=1.f) const;
    void display(ImVec2 display_size) const;
    ImVec2 size(float scale=1.f) const;
    ImVec2 size_with_width(float width, float scale=1.f) const;
    ImVec2 size_with_height(float height, float scale=1.f) const;
};

struct GuiAssets
{
    using Image = GuiImage; // remove this
    rhi::sampler_id default_sampler;
    void acquire(VkCommandBuffer cmd_buf);
    void release();
};

extern GuiAssets g_gui_assets;

using gui_image = rhi::image_id;
gui_image gui_load_image_2d_rgba(const char *filename);
gui_image gui_load_image_2d_array_tiff(const char *filename);

struct stb_image_data;
VkFormat stb_format(stb_image_data const& s);
quickgui::rhi::ImageLayout stb_layout(stb_image_data const& s);

struct imgview;
VkFormat imgview_format(int imgview_data_type_e, size_t num_channels);
VkFormat imgview_format(imgview const& s);
quickgui::rhi::ImageLayout imgview_layout(imgview const& s);

rhi::image_id load_image_2d_rgba(const char *filename, quickgui::rhi::ImageLayout const& layout, rhi::Rhi *rhi, VkCommandBuffer cmdbuf);
rhi::image_id load_image_2d_rgba(const char *filename, stb_image_data const&, rhi::Rhi *r, VkCommandBuffer cmd);
rhi::image_id load_image_2d_rgba(const char *filename, rhi::Rhi *rhi, VkCommandBuffer cmdbuf);
rhi::image_id load_image_2d_rgba(const char *filename, ccharspan file_contents, rhi::Rhi *rhi, VkCommandBuffer cmdbuf);

} // namespace quickgui

#endif /* QUICKGUI_GUI_GUI_HPP_ */
