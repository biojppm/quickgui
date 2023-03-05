
#include "quickgui/gui.hpp"
#include "quickgui/imgui_impl_sdl.h"
#include "quickgui/imgui_impl_vulkan.h"
#include "quickgui/static_vector.hpp"
#include "quickgui/stb_image_data.hpp"
#include "quickgui/imgview.hpp"
#include "quickgui/mem.hpp"
#include "quickgui/log.hpp"
#include "quickgui/sdl.hpp"

#include <c4/error.hpp>
#include <c4/szconv.hpp>
#include <c4/fs/fs.hpp>

#include <SDL_vulkan.h>

// FIXME
void SetupVulkan(const char **exts, uint32_t num_exts, quickgui::reusable_buffer *exts_buf);
void CleanupVulkan();
void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
void CleanupVulkanWindow();
bool FrameStart(ImGui_ImplVulkanH_Window* wd); // returns true if the swap chain should be rebuilt
void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
void FramePresent(ImGui_ImplVulkanH_Window* wd);


extern VkAllocationCallbacks*   g_Allocator;
extern VkInstance               g_Instance;
extern VkPhysicalDevice         g_PhysicalDevice;
extern VkDevice                 g_Device;
extern uint32_t                 g_QueueFamily;
extern VkQueue                  g_Queue;
extern VkDebugReportCallbackEXT g_DebugReport;
extern VkPipelineCache          g_PipelineCache;
extern VkDescriptorPool         g_DescriptorPool;
extern ImGui_ImplVulkanH_Window g_MainWindowData;
extern uint32_t                 g_MinImageCount;
extern bool                     g_SwapChainRebuild;

SDL_Window              *g_window = nullptr;
bool                     g_window_full_screen = false;
SDL_DisplayMode          g_window_display_mode = {};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace quickgui::gui {

struct GuiState
{
    using FrameEvents = quickgui::static_vector<SDL_Event, 64>;

    FrameEvents frame_events;

    GuiState() : frame_events() {}

    void iter_events(quickgui::GuiEventHandler fn, void *data)
    {
        for(auto & event : frame_events)
        {
            if(fn(event, data))
            {
                event = {};
                return;
            }
        }
    }

    void push_event(SDL_Event const& event)
    {
        frame_events.emplace_back(event);
    }

    void clear_events()
    {
        frame_events.clear();
    }
};

} // namespace quickgui::gui

static std::aligned_storage_t<sizeof(quickgui::gui::GuiState), alignof(quickgui::gui::GuiState)> g_gui_state_buf;
quickgui::gui::GuiState  &g_gui_state = reinterpret_cast<quickgui::gui::GuiState&>(g_gui_state_buf);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace quickgui {

VkFormat imgview_format(imgview const& iv)
{
    return imgview_format(iv.data_type, iv.num_channels);
}
VkFormat imgview_format(int imgview_data_type_e, size_t num_channels)
{
    static constexpr const VkFormat formats[] = {
        // u8
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        // i8
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8B8_SNORM,
        VK_FORMAT_R8G8B8A8_SNORM,
        // u32
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32B32A32_UINT,
        // i32
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32A32_SINT,
        // f32
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
    };
    static_assert(imgview::data_u8 == 0);
    static_assert(imgview::data_i8 == 4);
    static_assert(imgview::data_u32 == 8);
    static_assert(imgview::data_i32 == 12);
    static_assert(imgview::data_f32 == 16);
    static_assert(C4_COUNTOF(formats) >= 20);
    return formats[imgview_data_type_e + (int)num_channels];
}

quickgui::rhi::ImageLayout imgview_layout(imgview const& s)
{
    return quickgui::rhi::ImageLayout::make_2d(imgview_format(s), (uint32_t)s.width, (uint32_t)s.height);
}


VkFormat stb_format(stb_image_data const& s)
{
    switch(s.num_channels)
    {
    case 4:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case 3:
        return VK_FORMAT_R8G8B8_UNORM;
    case 2:
        return VK_FORMAT_R8G8_UNORM;
    case 1:
        return VK_FORMAT_R8_UNORM;
    }
    C4_ERROR("unknown format");
    return VK_FORMAT_UNDEFINED;
}

quickgui::rhi::ImageLayout stb_layout(stb_image_data const& s)
{
    return quickgui::rhi::ImageLayout::make_2d(stb_format(s), s.width, s.height);
}


#ifdef TODO
//! @warning this is crashing when loading an icon. investigate.
SDL_Surface* load_sdl_surface_2d(quickgui::ccharspan file_contents)
{
    stb_image_data img(file_contents, RequiredChannels::defer);
    int width = c4::szconv<int>(img.width);
    int height = c4::szconv<int>(img.height);
    int nch = c4::szconv<int>(img.num_channels);
    C4_CHECK(nch > 1);
    C4_CHECK(nch <= 4);
    static_assert(SDL_BYTEORDER != SDL_BIG_ENDIAN);
    uint32_t rmask =            0x000000ffu;
    uint32_t gmask = nch >= 2 ? 0x0000ff00u : 0;
    uint32_t bmask = nch >= 3 ? 0x00ff0000u : 0;
    uint32_t amask = nch == 4 ? 0xff000000u : 0;
    SDL_Surface * surface = SDL_CreateSurfaceFrom((void*)img.data,
        width, height, /*numBitsPerPixel*/nch*8, /*numBytesPerRow*/nch*width,
        rmask, gmask, bmask, amask);
    C4_CHECK_MSG(surface, "creating surface failed: %s", SDL_GetError());
    return surface;
}
#endif

 //-----------------------------------------------------------------------------
 //-----------------------------------------------------------------------------
 //-----------------------------------------------------------------------------

VkClearColorValue to_vk_clear_color(ImVec4 color)
{
    VkClearColorValue vk;
    vk.float32[0] = color.x;
    vk.float32[1] = color.y;
    vk.float32[2] = color.z;
    vk.float32[3] = color.w;
    return vk;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void set_full_screen(SDL_Window *window, bool value)
{
    if(value)
    {
        SDL_DisplayMode screen_display_mode = {};
        int curr_display = SDL_GetWindowDisplayIndex(window);
        SDL_GetCurrentDisplayMode(curr_display, &g_window_display_mode);
        SDL_GetDesktopDisplayMode(curr_display, &screen_display_mode);
        SDL_SetWindowDisplayMode(window, &screen_display_mode);
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }
    else
    {
        SDL_SetWindowDisplayMode(window, &g_window_display_mode);
        SDL_SetWindowFullscreen(window, 0);
    }
    SDL_ShowCursor(value);
    g_window_full_screen = value;
}

bool toggle_full_screen(SDL_Window *w)
{
    set_full_screen(w, !g_window_full_screen);
    return g_window_full_screen;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

rhi::image_id load_image_2d_rgba(const char *filename, rhi::ImageLayout const& layout, ccharspan data, rhi::Rhi *r, VkCommandBuffer cmd)
{
    auto img_id = r->make_image(layout.to_vk());
    r->set_name(img_id, filename);
    r->upload_image(img_id, layout, data, cmd);
    return img_id;
}

rhi::image_id load_image_2d_rgba(const char *filename, stb_image_data const& data, rhi::Rhi *r, VkCommandBuffer cmd)
{
    auto layout = stb_layout(data);
    return load_image_2d_rgba(filename, layout, data.to_span(), r, cmd);
}

rhi::image_id load_image_2d_rgba(const char *filename, ccharspan file_contents, rhi::Rhi *r, VkCommandBuffer cmd)
{
    stb_image_data data(file_contents, RequiredChannels::four);
    return load_image_2d_rgba(filename, data, r, cmd);
}

rhi::image_id load_image_2d_rgba(const char *filename, rhi::Rhi *r, VkCommandBuffer cmd)
{
    String buf = c4::fs::file_get_contents<String>(filename);
    ccharspan file_contents = {buf.data(), buf.size()};
    return load_image_2d_rgba(filename, file_contents, r, cmd);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


bool gui_init(GuiConfig const& cfg)
{
    new (&g_gui_state) gui::GuiState();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
        C4_ERROR("SDL init error: %s", SDL_GetError());

    // Setup SDL window
    const char* window_name = cfg.window_name.c_str();
    int window_width = cfg.window_width ? (int)cfg.window_width : 1280;
    int window_height = cfg.window_height ? (int)cfg.window_height :  720;
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    g_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height, window_flags);

    // Setup Vulkan
    reusable_buffer buf;
    uint32_t num_exts = 0;
    if(!SDL_Vulkan_GetInstanceExtensions(g_window, &num_exts, nullptr))
        C4_ERROR("could not get instance extensions: %s", SDL_GetError());
    const char** exts = buf.reset<const char*>(num_exts + 1);
    if(!SDL_Vulkan_GetInstanceExtensions(g_window, &num_exts, exts))
        C4_ERROR("could not get instance extensions: %s", SDL_GetError());
    exts[num_exts] = "VK_EXT_debug_report";
    SetupVulkan(exts, num_exts + 1, &buf);

    // Create Window Surface
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(g_window, g_Instance, &surface))
        C4_ERROR("failed to create Vulkan surface: %s", SDL_GetError());

    // Create Framebuffers
    int w, h;
    SDL_GetWindowSize(g_window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    ImPlot::StyleColorsDark();
    //ImPlot::StyleColorsLight();
    //ImPlot::SetColormap(ImPlotColormap_Deep);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(g_window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = g_Allocator;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.CheckVkResultFn = rhi::check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

    quickgui::rhi::rhi_init();
    gui_acquire_assets();

    return true;
}


void gui_terminate()
{
    C4_CHECK_VK(vkDeviceWaitIdle(g_Device));

    gui_release_assets();
    quickgui::rhi::rhi_terminate();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    CleanupVulkanWindow();
    CleanupVulkan();

    SDL_DestroyWindow(g_window);
    SDL_Quit();
    g_gui_state.~GuiState();
}


void gui_acquire_assets()
{
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

    // Use any command queue
    VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
    VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

    // reset the command buffer
    C4_CHECK_VK(vkResetCommandPool(g_Device, command_pool, 0));
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    C4_CHECK_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    g_gui_assets.acquire(command_buffer);

    // window icon WIP, not working
    //if(cfg.window_icon)
    //{
    //    auto icon_data = c4::fs::file_get_contents<String>(cfg.window_icon);
    //    SDL_Surface *icon_surface = load_sdl_surface_2d({icon_data.data(), icon_data.size()});
    //    SDL_SetSurfaceBlendMode(icon_surface, SDL_BLENDMODE_NONE);
    //    SDL_SetWindowIcon(g_window, icon_surface);
    //    SDL_FreeSurface(icon_surface);
    //}

    // finalize and submit the command buffer
    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &command_buffer;
    C4_CHECK_VK(vkEndCommandBuffer(command_buffer));
    C4_CHECK_VK(vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE));

    C4_CHECK_VK(vkDeviceWaitIdle(g_Device));
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}


void gui_release_assets()
{
    g_gui_assets.release();
}

void gui_iter_events(GuiEventHandler fn, void *data)
{
    g_gui_state.iter_events(fn, data);
}

bool gui_start_frame()
{
    // Poll and handle events (inputs, g_window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            return false;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(g_window))
            return false;
        if (event.type == SDL_KEYDOWN)
        {
            switch(event.key.keysym.sym)
            {
            case SDLK_f:
            case SDLK_F11:
                toggle_full_screen(g_window);
                g_SwapChainRebuild = true;
                break;
            case SDLK_RETURN:
                if(SDL_GetModState() & (KMOD_LALT|KMOD_RALT))
                {
                    toggle_full_screen(g_window);
                    g_SwapChainRebuild = true;
                }
                break;
            case SDLK_q:
                if(SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL))
                {
                    return false; // Ctrl-q: quit
                }
                break;
            default:
                g_gui_state.push_event(event);
                break;
            }
        }
    }

    if (g_SwapChainRebuild)
    {
        int width, height;
        SDL_GetWindowSize(g_window, &width, &height);
        if (width > 0 && height > 0)
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(g_window);
    ImGui::NewFrame();

    // start vulkan frame
    g_SwapChainRebuild |= FrameStart(&g_MainWindowData);

    return true;
}

void gui_end_frame(ImVec4 clear_color)
{
    g_gui_state.clear_events();
    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized)
    {
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        wd->ClearValue.color = to_vk_clear_color(clear_color);
        FrameRender(wd, draw_data);
        FramePresent(wd);
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

quickgui::GuiAssets g_gui_assets;

void GuiAssets::Image::load_existing(rhi::image_id img, rhi::sampler_id sampler)
{
    img_id = img;
    view_id = rhi::g_rhi.make_image_view(rhi::g_rhi.get_image(img_id));
    desc_set = ImGui_ImplVulkan_AddTexture(rhi::g_rhi.get_sampler(sampler), rhi::g_rhi.get_image_view(view_id));
}

void GuiAssets::Image::load(const char *filename, ccharspan img_data, rhi::ImageLayout const& layout, rhi::sampler_id sampler, VkCommandBuffer cmd_buf)
{
    img_id = load_image_2d_rgba(filename, layout, img_data, &rhi::g_rhi, cmd_buf);
    view_id = rhi::g_rhi.make_image_view(rhi::g_rhi.get_image(img_id));
    rhi::g_rhi.set_name(img_id, filename);
    rhi::g_rhi.set_name(view_id, filename);
    desc_set = ImGui_ImplVulkan_AddTexture(rhi::g_rhi.get_sampler(sampler), rhi::g_rhi.get_image_view(view_id));
}

void GuiAssets::Image::load(const char *filename, rhi::sampler_id sampler, VkCommandBuffer cmd_buf)
{
    img_id = load_image_2d_rgba(filename, &rhi::g_rhi, cmd_buf);
    view_id = rhi::g_rhi.make_image_view(rhi::g_rhi.get_image(img_id));
    rhi::g_rhi.set_name(img_id, filename);
    rhi::g_rhi.set_name(view_id, filename);
    desc_set = ImGui_ImplVulkan_AddTexture(rhi::g_rhi.get_sampler(sampler), rhi::g_rhi.get_image_view(view_id));
}

ImVec2 GuiAssets::Image::size(float scale) const
{
    auto const& rhi_img = rhi::g_rhi.get_image(img_id);
    return ImVec2((float)rhi_img.layout.width * scale, (float)rhi_img.layout.height * scale);
}

ImVec2 GuiAssets::Image::size_with_width(float width, float scale) const
{
    auto const& rhi_img = rhi::g_rhi.get_image(img_id);
    float w = (float)rhi_img.layout.width;
    float h = (float)rhi_img.layout.height;
    return ImVec2(width * scale, h * width / w * scale);
}

ImVec2 GuiAssets::Image::size_with_height(float height, float scale) const
{
    auto const& rhi_img = rhi::g_rhi.get_image(img_id);
    float w = (float)rhi_img.layout.width;
    float h = (float)rhi_img.layout.height;
    return ImVec2(w * height / h * scale, height * scale);
}

void GuiAssets::Image::display(float scale) const
{
    ImGui::Image((ImTextureID)desc_set, size(scale), uv_topl, uv_botr, tint_color, border_color);
}

void GuiAssets::Image::display(ImVec2 display_size) const
{
    ImGui::Image((ImTextureID)desc_set, display_size, uv_topl, uv_botr, tint_color, border_color);
}

void GuiAssets::acquire(VkCommandBuffer cmd_buf)
{
    default_sampler = rhi::g_rhi.make_sampler(rhi::g_rhi.build_sampler());
    //logo.load("./icons/logo.png", default_sampler, cmd_buf);
    (void)cmd_buf;
}

void GuiAssets::release()
{
    //logo = {};
    default_sampler = {};
}

} // namespace quickgui
