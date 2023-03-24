#ifndef QUICKGUI_GUI_RHI_HPP_
#define QUICKGUI_GUI_RHI_HPP_

#include <cstdint>
#include <cstddef>
#include <vector>
#include <vulkan/vulkan.h>
#include <c4/span.hpp>
#include "quickgui/static_vector.hpp"
#include "quickgui/string.hpp"
#include "quickgui/color.hpp"


#define C4_CHECK_VK(vk_call) quickgui::rhi::check_vk_result(vk_call)


namespace quickgui {
namespace rhi {

struct Rhi;
extern Rhi &g_rhi;
void rhi_init();
void rhi_terminate();


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void check_vk_result(VkResult err);

uint32_t vk_num_bytes_per_pixel(VkFormat fmt);
uint32_t vk_mem_type(VkMemoryPropertyFlags properties, uint32_t type_bits);

void enable_vk_debug(bool yes);

void debug_marker_setup(VkDevice device);
void debug_marker_set_name(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name);
void debug_marker_mark(VkCommandBuffer cmd, const char *name, fcolor const& C4_RESTRICT color);
void debug_marker_region_begin(VkCommandBuffer cmd, const char *name, fcolor const& C4_RESTRICT color);
void debug_marker_region_end(VkCommandBuffer cmd);

inline void debug_marker_mark(VkCommandBuffer cmd, const char *name, uint32_t color=UINT32_C(0x7f'7f'7f'ff))
{
    debug_marker_mark(cmd, name, fcolor(color));
}

inline void debug_marker_region_begin(VkCommandBuffer cmd, const char *name, uint32_t color=UINT32_C(0x7f'7f'7f'ff))
{
    debug_marker_region_begin(cmd, name, fcolor(color));
}

template<class vk_resource_type_>
inline void debug_marker_set_name(VkDevice device, vk_resource_type_ handle, const char *name)
{
    using restype = std::remove_const_t<std::remove_reference_t<vk_resource_type_>>;
    if constexpr(std::is_same_v<restype, VkDeviceMemory>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
    }
    else if constexpr(std::is_same_v<restype, VkFence>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, name);
    }
    else if constexpr(std::is_same_v<restype, VkBuffer>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
    }
    else if constexpr(std::is_same_v<restype, VkImage>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
    }
    else if constexpr(std::is_same_v<restype, VkImageView>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, name);
    }
    else if constexpr(std::is_same_v<restype, VkSampler>)
    {
        debug_marker_set_name(device, (uint64_t)handle, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name);
    }
    else
    {
        C4_STATIC_ERROR(restype, "unknown resource type");
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct SamplerBuilder
{
    VkSamplerCreateInfo info;
    inline operator VkSamplerCreateInfo const& () const { return info; }
    SamplerBuilder() : info()
    {
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
    }
    SamplerBuilder &mag_filter(VkFilter mode) { info.magFilter = mode; return *this; }
    SamplerBuilder &min_filter(VkFilter mode) { info.minFilter = mode; return *this; }
    SamplerBuilder &filter(VkFilter mode) { info.magFilter = info.minFilter = mode; return *this; }
    SamplerBuilder &mipmap(VkSamplerMipmapMode mode) { info.mipmapMode = mode; return *this; }
    SamplerBuilder &address_u(VkSamplerAddressMode mode) { info.addressModeU = mode; return *this; }
    SamplerBuilder &address_v(VkSamplerAddressMode mode) { info.addressModeV = mode; return *this; }
    SamplerBuilder &address_w(VkSamplerAddressMode mode) { info.addressModeW = mode; return *this; }
    SamplerBuilder &address(VkSamplerAddressMode mode) { info.addressModeU = info.addressModeV = info.addressModeW = mode; return *this; }
};


inline VkExtent3D region_extent(VkOffset3D first, VkOffset3D last)
{
    C4_ASSERT(last.x >= first.x);
    C4_ASSERT(last.y >= first.y);
    C4_ASSERT(last.z >= first.z);
    VkExtent3D ret;
    ret.width = uint32_t(last.x - first.x);
    ret.height = uint32_t(last.y - first.y);
    ret.depth = uint32_t(last.z - first.z);
    return ret;
}

inline int32_t region_offset(VkOffset3D region_offs, VkExtent3D img_xt)
{
    return region_offs.x
        + region_offs.y * (int32_t)img_xt.width
        + region_offs.z * ((int32_t)img_xt.width * (int32_t)img_xt.height);
}

inline uint32_t region_size(VkOffset3D region_begin, VkOffset3D region_end, VkExtent3D img_xt)
{
    VkExtent3D xt = region_extent(region_begin, region_end);
    return xt.width
        + xt.height * img_xt.width
        + xt.depth * (img_xt.width * img_xt.height);
}


struct ImageLayout
{
    VkFormat format = {};
    uint32_t width = {};
    uint32_t height = {};
    uint32_t depth = 1; // for texture arrays
    bool     force_tex_array = true;

    void clear() { memset(this, 0, sizeof(ImageLayout)); }

    bool is_2d_array() const { return depth > 1 || force_tex_array; }
    bool is_2d() const { return !is_2d_array(); }

    size_t num_bytes() const { return width * height * depth * vk_num_bytes_per_pixel(format); }
    size_t num_bytes_per_pixel() const { return vk_num_bytes_per_pixel(format); }
    VkImageType image_type() const { return VK_IMAGE_TYPE_2D; }
    VkImageViewType view_type() const { return is_2d_array() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D; }
    VkExtent3D extent() const { VkExtent3D xt; xt.width = width; xt.height = height; xt.depth = depth; return xt; }

    static ImageLayout make_2d(VkFormat fmt, uint32_t w, uint32_t h)
    {
        ImageLayout tex;
        tex.format = fmt;
        tex.width = w;
        tex.height = h;
        tex.depth = 1;
        tex.force_tex_array = false;
        return tex;
    }

    static ImageLayout make_2d_array(VkFormat fmt, uint32_t w, uint32_t h, uint32_t d)
    {
        ImageLayout tex;
        tex.format = fmt;
        tex.width = w;
        tex.height = h;
        tex.depth = d;
        tex.force_tex_array = true;
        return tex;
    }

    static ImageLayout make(VkImageCreateInfo const& nfo)
    {
        C4_CHECK((nfo.arrayLayers > 1) != (nfo.extent.depth > 1) || nfo.arrayLayers == 1);
        ImageLayout tex;
        tex.format = nfo.format;
        tex.width = nfo.extent.width;
        tex.height = nfo.extent.height;
        tex.depth = nfo.arrayLayers > 1 ? nfo.arrayLayers : nfo.extent.depth;
        tex.force_tex_array = nfo.arrayLayers > 1;
        return tex;
    }

    VkImageCreateInfo to_vk() const
    {
        VkImageCreateInfo nfo = {};
        nfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        nfo.imageType = VK_IMAGE_TYPE_2D;
        nfo.format = format;
        nfo.extent.width = width;
        nfo.extent.height = height;
        nfo.extent.depth = 1;
        nfo.mipLevels = 1;
        nfo.arrayLayers = depth;
        nfo.samples = VK_SAMPLE_COUNT_1_BIT;
        nfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        nfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        nfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        nfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return nfo;
    }
};


/** the image memory should generally be pooled in a few
 * blocks per application. For our use case, it should not matter
 * much, so we allocate one memory block per texture */
struct Image
{
    VkImage handle = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    ImageLayout layout = {};
    void create(VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_type, VkDevice v, VkAllocationCallbacks const* a);
    void create(ImageLayout const& C4_RESTRICT layout, uint32_t mem_type, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(VkDevice v, VkAllocationCallbacks const* a);
    void destroy_mem(VkDevice v, VkAllocationCallbacks const* a);
    inline operator VkImage () const { return handle; }
    inline operator bool () const { return handle != VK_NULL_HANDLE; }
};


/** the buffer memory should generally be pooled in a few
 * blocks per application. For our use case, it should not matter
 * much, so we allocate one memory block per texture */
struct Buffer
{
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDeviceSize size = {};
    VkDeviceSize alignment = {};
    void create(VkBufferCreateInfo const& nfo, uint32_t mem_type, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(VkDevice v, VkAllocationCallbacks const* a);
    void destroy_mem(VkDevice v, VkAllocationCallbacks const* a);
    inline operator VkBuffer () const { return handle; }
    inline operator bool () const { return handle != VK_NULL_HANDLE; }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<class HandleType>
struct HandleCollection
{
    /** an opaque type-safe id */
    class id_type
    {
        friend struct HandleCollection;
    protected:
        size_t _id = (size_t)-1;
    public:
        inline operator bool () const { return _id != (size_t)-1; }
    };

public:

    using handle_type = HandleType;
    std::vector<HandleType> handles;

    template<class ...CtorArgs>
    id_type add_handle(CtorArgs ...args)
    {
        id_type id;
        id._id = handles.size();
        handles.emplace_back(std::forward<CtorArgs>(args)...);
        return id;
    }

    //! short-lived references! referencial integrity is not guaranteed,
    //! as an insertion may change the reference.
    HandleType const& get_handle(id_type id) const { return handles[id._id]; }
    //! short-lived references! referencial integrity is not guaranteed,
    //! as an insertion may change the reference.
    HandleType & get_handle(id_type id) { return handles[id._id]; }

    virtual ~HandleCollection()
    {
        for(HandleType const& h : handles)
            C4_CHECK(!h);
    }
};


struct FenceCollection : public HandleCollection<VkFence>
{
    id_type reset(id_type id, VkFenceCreateInfo const& info, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(id_type id, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy_all(VkDevice dev, VkAllocationCallbacks const* alloc);
};
using fence_id = FenceCollection::id_type;


struct BufferCollection : public HandleCollection<Buffer>
{
    id_type reset(id_type id, VkBufferCreateInfo const& C4_RESTRICT info, uint32_t mem_bits, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(id_type id, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy_all(VkDevice dev, VkAllocationCallbacks const* alloc);
};
using buffer_id = BufferCollection::id_type;


struct ImageCollection : public HandleCollection<Image>
{
    id_type reset(id_type id, VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_bits, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(id_type id, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy_all(VkDevice dev, VkAllocationCallbacks const* alloc);
};
using image_id = ImageCollection::id_type;


struct ImageViewCollection : public HandleCollection<VkImageView>
{
    id_type reset(id_type id, Image const& img, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(id_type id, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy_all(VkDevice dev, VkAllocationCallbacks const* alloc);
};
using image_view_id = ImageViewCollection::id_type;


struct SamplerCollection : public HandleCollection<VkSampler>
{
    id_type reset(id_type id, VkSamplerCreateInfo const& info, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy(id_type id, VkDevice dev, VkAllocationCallbacks const* alloc);
    void destroy_all(VkDevice dev, VkAllocationCallbacks const* alloc);
};
using sampler_id = SamplerCollection::id_type;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** Render Hardware Interface */
struct Rhi
{
    VkDevice                      m_device;
    VkPhysicalDevice              m_phys_device;
    VkAllocationCallbacks const  *m_allocator;
    Buffer                        m_upload_buffer; // should we put this into the buffer collection?
    FenceCollection               m_fences;
    ImageViewCollection           m_image_views;
    SamplerCollection             m_samplers;
    ImageCollection               m_images;
    BufferCollection              m_buffers;
    size_t                        m_non_coherent_atom_size;

    Rhi();
    Rhi(VkDevice device, VkPhysicalDevice phys_device, VkAllocationCallbacks const* allocator);
    ~Rhi();

    size_t required_buffer_size(size_t wanted) const;
    size_t resize_upload_buffer(size_t upload_size);

    // fences
    [[nodiscard]] fence_id make_fence(VkFenceCreateInfo const& info) { return m_fences.reset({}, info, m_device, m_allocator); }
    [[nodiscard]] fence_id reset_fence(fence_id id, VkFenceCreateInfo const& info) { return m_fences.reset(id, info, m_device, m_allocator); }
    void          destroy_fence(fence_id id) { return m_fences.destroy(id, m_device, m_allocator); }
    VkFence       get_fence(fence_id id)        { return m_fences.get_handle(id); }
    VkFence       get_fence(fence_id id) const  { return m_fences.get_handle(id); }
    void          set_name (fence_id id     , const char *name) { debug_marker_set_name(m_device, get_fence(id), name); }
    void          set_name (VkFence f, const char *name) { debug_marker_set_name(m_device, f, name); }

    // buffers
    [[nodiscard]] buffer_id make_buffer(VkBufferCreateInfo const& info, uint32_t mem_bits) { return m_buffers.reset({}, info, mem_bits, m_device, m_allocator); }
    [[nodiscard]] buffer_id reset_buffer(buffer_id id, VkBufferCreateInfo const& info, uint32_t mem_bits) { return m_buffers.reset(id, info, mem_bits, m_device, m_allocator); }
    void          destroy_buffer(buffer_id id) { return m_buffers.destroy(id, m_device, m_allocator); }
    Buffer      & get_buffer(buffer_id id)        { return m_buffers.get_handle(id); }
    Buffer const& get_buffer(buffer_id id) const  { return m_buffers.get_handle(id); }
    void          set_name  (buffer_id id     , const char *name) { debug_marker_set_name(m_device, get_buffer(id).handle, name); }
    void          set_name  (Buffer const& buf, const char *name) { debug_marker_set_name(m_device, buf.handle, name); }

    // images
    [[nodiscard]] image_id make_image(VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_bits=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) { return m_images.reset({}, nfo, mem_bits, m_device, m_allocator); }
    [[nodiscard]] image_id reset_image(image_id id, VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_bits=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) { return m_images.reset(id, nfo, mem_bits, m_device, m_allocator); }
    void         destroy_image(image_id id) { return m_images.destroy(id, m_device, m_allocator); }
    Image      & get_image   (image_id id)        { return m_images.get_handle(id); }
    Image const& get_image   (image_id id) const  { return m_images.get_handle(id); }
    void         set_name    (image_id id     , const char *name) { debug_marker_set_name(m_device, get_image(id).handle, name); }
    void         set_name    (Image const& img, const char *name) { debug_marker_set_name(m_device, img.handle, name); }
    void         upload_image(image_id id, ImageLayout const& layout, ccharspan tex_data, VkCommandBuffer cmdbuf);

    // image views
    [[nodiscard]] image_view_id make_image_view(Image const& img) { return m_image_views.reset({}, img, m_device, m_allocator); }
    [[nodiscard]] image_view_id reset_image_view(image_view_id id, Image const& img) { return m_image_views.reset(id, img, m_device, m_allocator); }
    void               destroy_image_view(image_view_id id) { return m_image_views.destroy(id, m_device, m_allocator); }
    VkImageView      & get_image_view(image_view_id id)       { return m_image_views.get_handle(id); }
    VkImageView const& get_image_view(image_view_id id) const { return m_image_views.get_handle(id); }
    void               set_name      (image_view_id id, const char *name) { debug_marker_set_name(m_device, get_image_view(id), name); }
    void               set_name      (VkImageView view, const char *name) { debug_marker_set_name(m_device, view, name); }

    // samplers
    SamplerBuilder   build_sampler() const { return SamplerBuilder(); }
    [[nodiscard]] sampler_id make_sampler(SamplerBuilder const& b) { return m_samplers.reset({}, b, m_device, m_allocator); }
    [[nodiscard]] sampler_id reset_sampler(sampler_id id, SamplerBuilder const& b) { return m_samplers.reset(id, b, m_device, m_allocator); }
    void             destroy_sampler(sampler_id id) { return m_samplers.destroy(id, m_device, m_allocator); }
    VkSampler      & get_sampler (sampler_id id)       { return m_samplers.get_handle(id); }
    VkSampler const& get_sampler (sampler_id id) const { return m_samplers.get_handle(id); }
    void             set_name    (sampler_id id       , const char *name) { debug_marker_set_name(m_device, get_sampler(id), name); }
    void             set_name    (VkSampler const& smp, const char *name) { debug_marker_set_name(m_device, smp, name); }

    // HACK
    VkCommandBuffer usr_cmd_buffer();
    void            mark_usr_cmd_buffer();

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** an image that is frequently updated from the CPU
 *
 * @see https://stackoverflow.com/questions/40574668/how-to-update-texture-for-every-frame-in-vulkan/40575629
 */
struct ImageDynamicCpu2Gpu
{
    static inline constexpr const uint32_t num_entries = 2;

    Rhi        *rhi = {};
    fence_id    fence_ids[num_entries] = {};
    image_id    img_ids[num_entries] = {};
    buffer_id   buf_ids[num_entries] = {};
    charspan    bufmem[num_entries] = {}; ///< the mapped buffer memory
    uint32_t    wcpu = {}; ///< the index of the image/buffer where the CPU is currently writing
    uint32_t    rgpu = {}; ///< the index of the image/buffer where the GPU is currently reading

    void     set_name(Rhi *rhi, const char *name);
    void     reset(Rhi *rhi, VkImageCreateInfo const& C4_RESTRICT nfo);
    charspan start_wcpu();
    void     finish_wcpu(ccharspan written);
    charspan start_wcpu(VkOffset3D first, VkOffset3D last);
    void     finish_wcpu(VkOffset3D first, VkOffset3D last, ccharspan written);
    void     flip()
    {
        wcpu = (wcpu + 1) % num_entries;
        rgpu = (rgpu + 1) % num_entries;
    }
    ImageLayout const& layout() const { return rhi->get_image(img_ids[rgpu]).layout; }
    VkFence  fence(uint32_t which) { C4_ASSERT(which < num_entries); C4_ASSERT(fence_ids[which]); return rhi->get_fence(fence_ids[which]); }
    Buffer&  buf(uint32_t which) { C4_ASSERT(which < num_entries); C4_ASSERT(buf_ids[which]); return rhi->get_buffer(buf_ids[which]); }
    Image&   img(uint32_t which) { C4_ASSERT(which < num_entries); C4_ASSERT(img_ids[which]); return rhi->get_image(img_ids[which]); }
    Image&   img_rgpu() { return img(wcpu); }
    Image&   img_wgpu() { return img(rgpu); }
};

} // namespace rhi
} // namespace quickgui

#endif /* QUICKGUI_GUI_RHI_HPP_ */
