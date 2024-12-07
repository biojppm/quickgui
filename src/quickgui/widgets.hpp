#ifndef QUICKGUI_GUI_WIDGETS_HPP_
#define QUICKGUI_GUI_WIDGETS_HPP_

#include "quickgui/time.hpp"
#include "quickgui/gui.hpp"
#include <math.h>

C4_SUPPRESS_WARNING_MSVC_WITH_PUSH(4127) // conditional expression is constant

namespace quickgui {

namespace widgets {

C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

//-----------------------------------------------------------------------------

inline void hspace(float px) { ImGui::Dummy(ImVec2(px, 0.f)); }
inline void vspace(float px) { ImGui::Dummy(ImVec2(0.f, px)); }
inline void space(ImVec2 px) { ImGui::Dummy(px); }

void push_disabled();
void pop_disabled();

void push_disabled(bool disabled);
void pop_disabled(bool disabled);

void push_enabled(bool enabled);
void pop_enabled(bool enabled);

void push_compact();
void pop_compact();

struct DisabledScope
{
    bool disabled;
    DisabledScope(bool disabled_) : disabled(disabled_) { push_disabled(disabled); }
    ~DisabledScope() { pop_disabled(disabled); }
};

struct EnabledScope
{
    bool enabled;
    EnabledScope(bool enabled_) : enabled(enabled_) { push_enabled(enabled); }
    ~EnabledScope() { pop_enabled(enabled); }
};

struct CompactScope
{
    CompactScope() { push_compact(); }
    ~CompactScope() { pop_compact(); }
};


bool toggle_button(const char *btn_txt, bool *var);

void set_hover_tooltip(const char *tooltip);
void set_tooltip(const char *tooltip);

/** returns true if the checkbox was changed */
bool checkbox_negated(const char *title, bool *var);

bool compact_color_picker(const char * title, ucolor *color, const char *tooltip=nullptr);
bool compact_color_picker(const char * title, fcolor *color, const char *tooltip=nullptr);

ImRect get_current_plot_rect();

void set_column_padding_for_aligned_right_contents(c4::csubstr txt);
void set_column_contents_aligned_right(c4::csubstr txt);
void set_column_contents_aligned_right(int col, c4::csubstr txt);


//-----------------------------------------------------------------------------

struct FilePicker
{
    ImGuiFs::Dialog dialog = {};
    /// @p file_filter can be something like ".png;.jpg;.jpeg;.bmp;.tga;.gif;.tiff;.txt". It's case insensitive.
    const char *pick_file(const char* btn_txt="Select", const char *file_filter="", const char *window_title="Select")
    {
        return pick_file_(dialog.getLastDirectory(), btn_txt, file_filter, window_title);
    }
    const char *pick_file_(const char *directory, const char* btn_txt="Select", const char *file_filter="", const char *window_title="Select")
    {
        bool open = ImGui::Button(btn_txt);
        set_hover_tooltip("Click to select");
        const char *path = dialog.chooseFileDialog(open, directory, file_filter, window_title);
        if(dialog.hasUserJustCancelledDialog() || !path || strlen(path) == 0)
            return nullptr;
        return path;
    }
    const char *pick_dir(const char* btn_txt="Select", const char *dir=nullptr)
    {
        bool open = ImGui::Button(btn_txt);
        set_hover_tooltip("Click to select");
        const char *path = dialog.chooseFolderDialog(open, dir ? dir : dialog.getLastDirectory());
        if(dialog.hasUserJustCancelledDialog() || !path || strlen(path) == 0)
            return nullptr;
        return path;
    }
    const char *pick_dir_(const char *directory, const char* btn_txt="Select")
    {
        bool open = ImGui::Button(btn_txt);
        set_hover_tooltip("Click to select");
        const char *path = dialog.chooseFolderDialog(open, directory);
        if(dialog.hasUserJustCancelledDialog() || !path || strlen(path) == 0)
            return nullptr;
        return path;
    }
};


//-----------------------------------------------------------------------------

template<class DurationGui, class DurationValue>
bool duration_input_double(const char *title, DurationValue *value)
{
    DurationGui dg = std::chrono::duration_cast<DurationGui>(*value);
    double val = (double)dg.count();
    if(ImGui::InputDouble(title, &val, 1.0, 10.0, "%f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        dg = DurationGui(val);
        *value = std::chrono::duration_cast<DurationValue>(dg);
        return true;
    }
    return false;
}

template<class DurationGui, class DurationValue>
bool duration_input_integer(const char *title, DurationValue *value)
{
    DurationGui dg = std::chrono::duration_cast<DurationGui>(*value);
    int val = (int)dg.count();
    if(ImGui::InputInt(title, &val, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        dg = DurationGui(val);
        *value = std::chrono::duration_cast<DurationValue>(dg);
        return true;
    }
    return false;
}

template<class DurationGui, class DurationValue>
bool duration_drag_float(const char *title, DurationValue *value, DurationValue min, DurationValue max, const char *fmt="%.3f")
{
    DurationGui dg = std::chrono::duration_cast<DurationGui>(*value);
    float dgmax = (float)std::chrono::duration_cast<DurationGui>(max).count();
    float dgmin = (float)std::chrono::duration_cast<DurationGui>(min).count();
    float val = static_cast<float>(dg.count());
    if(ImGui::SliderFloat(title, &val, dgmin, dgmax, fmt))
    {
        dg = DurationGui((typename DurationGui::rep)val);
        *value = std::chrono::duration_cast<DurationValue>(dg);
        return true;
    }
    return false;
}



/** used for the plots */
template<class T>
struct ScrollingBuffer
{
    fixed_vector<T> data;
    size_t pos;
    bool full;
    ScrollingBuffer(size_t sz=2048) : data(sz), pos(0), full(false) { C4_CHECK(is_pot(sz)); }
    void clear() { pos = 0; full = false; }
    T const& back() const { return (*this)[0]; }
    T& add()
    {
        size_t prev = pos++;
        full |= pos == data.size();
        pos &= (data.size() - size_t(1));
        return data[prev];
    }
    void add(T const& val)
    {
        data[pos++] = val;
        full |= pos == data.size();
        pos &= (data.size() - size_t(1));
    }
    size_t size() const { return full ? data.size() : pos; }
    // 0 is the latest value, 1 the second-most latest value and so on.
    T      & operator[] (size_t i)       { C4_ASSERT(i < data.size()); return data[i + 1 <= pos ? pos - (i + 1) : (data.size() - ((i + 1) - pos))]; }
    T const& operator[] (size_t i) const { C4_ASSERT(i < data.size()); return data[i + 1 <= pos ? pos - (i + 1) : (data.size() - ((i + 1) - pos))]; }
};

} // namespace widgets


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct GuiImgDisplayData
{
    //! @todo: add zoom and anchors
    ImVec2 size = {1000.f, -1.f};
    ImVec2 actual_size(GuiAssets::Image const& img) const
    {
        auto eq = [](float lhs, float rhs){ return std::fabs(lhs-rhs) < 1.e-3f; };
        if(eq(size.x, -1.f) && eq(size.y, -1.f))
            return img.size();
        else if(eq(size.x, -1.f))
            return img.size_with_height(size.y);
        else if(eq(size.y, -1.f))
            return img.size_with_width(size.x);
        return img.size();
    }
};



//! an image that is continuously updated from the CPU
struct DynamicImage
{
    using RhiImage = rhi::ImageDynamicCpu2Gpu;
    RhiImage rhi_img = {};
    GuiAssets::Image gui_img[RhiImage::num_entries] = {};
    size_t flip_count = 0;
public:
    void set_name(const char* name);
    void reset(uint32_t width, uint32_t height, VkFormat format);
    bool ready_for_display() const { return flip_count > RhiImage::num_entries; }
    GuiAssets::Image      & curr_img()       { return gui_img[rhi_img.rgpu]; }
    GuiAssets::Image const& curr_img() const { return gui_img[rhi_img.rgpu]; }
    size_t width() const { return curr_img().layout().width; }
    size_t height() const { return curr_img().layout().height; }
    void flip();
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace widgets {

// TODO: move all the static state in the functions to this structure
struct WidgetState
{
    bool show_vulkan_window = false;
    // VULKAN
    bool debug_vulkan = false;
    // DEMOS
    bool show_demo_window = false;
    bool show_demo_widget = false;
    bool show_plot_demo_widget = false;
    //
    ImVec4 clear_color = {1.f/255.f, 19.f/255.f, 26.f/255.f, 1.f}; // the background color
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


void vulkan_window(WidgetState *st);
void events(WidgetState *st);

void demo(WidgetState *st);

void draw_bg(quickgui::GuiAssets::Image const& img, ImVec2 topleft, ImVec2 pivot);
inline void draw_bg()
{
    //draw_bg(quickgui::g_gui_assets.logo, /*topleft*/{5.f, 0.f}, /*pivot*/{0.f, 0.f});
}

C4_SUPPRESS_WARNING_GCC_CLANG_POP

} // namespace widgets
} // namespace quickgui

C4_SUPPRESS_WARNING_MSVC_POP

#endif /* QUICKGUI_GUI_WIDGETS_HPP_ */
