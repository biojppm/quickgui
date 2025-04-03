
#include "quickgui/rhi.hpp"
#include "quickgui/mem.hpp"
#include "quickgui/math.hpp"
#include "quickgui/log.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <c4/format.hpp>
#include <c4/szconv.hpp>
#include <c4/fs/fs.hpp>
#include "quickgui/sdl.hpp"
#include <SDL_vulkan.h>
#include <numeric>

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifndef QUICKGUI_ENABLE_VULKAN_DEBUG
#if defined(_DEBUG) || !defined(NDEBUG)
#define QUICKGUI_ENABLE_VULKAN_DEBUG
#endif
#endif


C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wold-style-cast")

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// FIXME
VkAllocationCallbacks*   g_Allocator = NULL;
VkInstance               g_Instance = VK_NULL_HANDLE;
VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
VkDevice                 g_Device = VK_NULL_HANDLE;
uint32_t                 g_QueueFamily = (uint32_t)-1;
VkQueue                  g_Queue = VK_NULL_HANDLE;
VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
ImGui_ImplVulkanH_Window g_MainWindowData;
uint32_t                 g_MinImageCount = 2;
bool                     g_SwapChainRebuild = false;

namespace quickgui::rhi {

std::aligned_storage_t<sizeof(Rhi), alignof(quickgui::rhi::Rhi)> g_rhi_buf;
Rhi &g_rhi = reinterpret_cast<Rhi&>(g_rhi_buf);

void rhi_init()
{
    new (&g_rhi_buf) Rhi(g_Device, g_PhysicalDevice, g_Allocator);
}

void rhi_terminate()
{
    g_rhi.~Rhi();
}

} // namespace quickgui::rhi


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


#ifdef QUICKGUI_ENABLE_VULKAN_DEBUG

c4::csubstr debug_report_object_type_name(VkDebugReportObjectTypeEXT type) noexcept
{
    switch(type)
    {
    #define _(n) case VK_DEBUG_REPORT_OBJECT_TYPE_##n##_EXT: return c4::csubstr(#n)
    _(UNKNOWN);
    _(INSTANCE);
    _(PHYSICAL_DEVICE);
    _(DEVICE);
    _(QUEUE);
    _(SEMAPHORE);
    _(COMMAND_BUFFER);
    _(FENCE);
    _(DEVICE_MEMORY);
    _(BUFFER);
    _(IMAGE);
    _(EVENT);
    _(QUERY_POOL);
    _(BUFFER_VIEW);
    _(IMAGE_VIEW);
    _(SHADER_MODULE);
    _(PIPELINE_CACHE);
    _(PIPELINE_LAYOUT);
    _(RENDER_PASS);
    _(PIPELINE);
    _(DESCRIPTOR_SET_LAYOUT);
    _(SAMPLER);
    _(DESCRIPTOR_POOL);
    _(DESCRIPTOR_SET);
    _(FRAMEBUFFER);
    _(COMMAND_POOL);
    _(SURFACE_KHR);
    _(SWAPCHAIN_KHR);
    _(DEBUG_REPORT_CALLBACK_EXT);
    _(DISPLAY_KHR);
    _(DISPLAY_MODE_KHR);
    _(VALIDATION_CACHE);
    _(SAMPLER_YCBCR_CONVERSION);
    _(DESCRIPTOR_UPDATE_TEMPLATE);
    //_(CU_MODULE_NVX);
    //_(CU_FUNCTION_NVX);
    //_(ACCELERATION_STRUCTURE_KHR);
    //_(ACCELERATION_STRUCTURE_NV);
    //_(BUFFER_COLLECTION_FUCHSIA);
    //_(DEBUG_REPORT);
    //_(VALIDATION_CACHE);
    //_(DESCRIPTOR_UPDATE_TEMPLATE_KHR);
    //_(SAMPLER_YCBCR_CONVERSION_KHR);
    //_(MAX_ENUM);
    #undef _
    default:
        return c4::csubstr("unknown?");
    };
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_report_vk(VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT objectType,
                uint64_t object,
                size_t location,
                int32_t messageCode,
                const char* pLayerPrefix,
                const char* pMessage,
                void* pUserData)
{
    C4_UNUSED(pUserData);
    quickgui::detail::_logfileline(__FILE__, __LINE__);
    quickgui::detail::_logtimestamp();
    quickgui::detail::_logf("[vulkan");
    struct flagandname { VkDebugReportFlagsEXT val; const char* name; };
    for(flagandname flag : {
            flagandname{VK_DEBUG_REPORT_ERROR_BIT_EXT, "ERR"},
            flagandname{VK_DEBUG_REPORT_WARNING_BIT_EXT, "WARN"},
            flagandname{VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, "PERF"},
            flagandname{VK_DEBUG_REPORT_INFORMATION_BIT_EXT, "INFO"},
            flagandname{VK_DEBUG_REPORT_DEBUG_BIT_EXT, "DBG"}})
        if(flags & flag.val)
            quickgui::detail::_logf("|{}", flag.name);
    quickgui::detail::_logf("] msg='{}'\n", c4::to_csubstr(pMessage));
    quickgui::detail::_logf("  msgcode={}\n", messageCode);
    quickgui::detail::_logf("  layer={}\n", c4::to_csubstr(pLayerPrefix));
    quickgui::detail::_logf("  location={}\n", c4::fmt::hex(location));
    quickgui::detail::_logf("  object={}\n", c4::fmt::hex(object));
    quickgui::detail::_logf("  objectType={}\n", debug_report_object_type_name(objectType));
    return VK_FALSE;
}
#endif // QUICKGUI_ENABLE_VULKAN_DEBUG


#define C4_DESTROY_VK(fn, device, handle, alloc) \
{\
    if((handle) != VK_NULL_HANDLE)\
    {\
        (fn)((device), (handle), (alloc));\
        (handle) = VK_NULL_HANDLE;\
    }\
}

#define C4_DESTROY_MEM_VK(device, handle, alloc) \
    C4_DESTROY_VK(vkFreeMemory, (device), (handle), (alloc))


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4_SUPPRESS_WARNING_MSVC_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG_PUSH
C4_SUPPRESS_WARNING_GCC_CLANG("-Wconversion")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wsign-conversion")
C4_SUPPRESS_WARNING_GCC_CLANG("-Wdouble-promotion")
C4_SUPPRESS_WARNING_GCC("-Wuseless-cast")


void SetupVulkan(const char **exts, uint32_t num_exts, quickgui::reusable_buffer *exts_buf)
{
    // TODO: debug markers:
    // https://www.saschawillems.de/blog/2016/05/28/tutorial-on-using-vulkans-vk_ext_debug_marker-with-renderdoc/

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.enabledExtensionCount = num_exts;
        create_info.ppEnabledExtensionNames = exts;
        #ifndef QUICKGUI_ENABLE_VULKAN_DEBUG  // create Vulkan Instance without any validation layer
        C4_CHECK_VK(vkCreateInstance(&create_info, g_Allocator, &g_Instance));
        #else   // enable validation layers
        // Enabling validation layers
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        C4_CHECK_VK(vkCreateInstance(&create_info, g_Allocator, &g_Instance));
        quickgui::rhi::enable_vk_debug(true);
        #endif
    }

    // Select GPU
    {
        uint32_t gpu_count;
        C4_CHECK_VK(vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr));
        C4_CHECK(gpu_count > 0);

        VkPhysicalDevice* gpus = exts_buf->reset<VkPhysicalDevice>(gpu_count);
        C4_CHECK_VK(vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus));

        // If a number >1 of GPUs got reported, you should find the
        // best fit GPU for your purpose
        // e.g. VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU if available, or
        // with the greatest memory available, etc.  for sake of
        // simplicity we'll just take the first one, assuming it has a
        // graphics queue family.
        g_PhysicalDevice = gpus[0];
    }

    // Select graphics queue family
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, NULL);
        VkQueueFamilyProperties* queues = exts_buf->reset<VkQueueFamilyProperties>(count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
        for (uint32_t i = 0; i < count; i++)
        {
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                g_QueueFamily = i;
                break;
            }
        }
        C4_CHECK(g_QueueFamily != (uint32_t)-1);
    }

    // Create Logical Device (with 1 queue)
    {
        // device extensions
        const char** devexts = exts_buf->reset<const char*>(2);
        uint32_t devexts_count = 0;
        devexts[devexts_count++] = "VK_KHR_swapchain";
        #ifdef QUICKGUI_ENABLE_VULKAN_DEBUG
        devexts[devexts_count++] = VK_EXT_DEBUG_MARKER_EXTENSION_NAME;
        #endif
        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = devexts_count;
        create_info.ppEnabledExtensionNames = devexts;
        C4_CHECK_VK(vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device));
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
        quickgui::rhi::debug_marker_setup(g_Device);
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * C4_COUNTOF(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)C4_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        C4_CHECK_VK(vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool));
    }
}

void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

    #ifdef QUICKGUI_ENABLE_VULKAN_DEBUG
    quickgui::rhi::enable_vk_debug(false);
    #endif

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    wd->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
    if (res != VK_TRUE)
    {
        C4_ERROR("No WSI support on physical device 0");
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM,
        VK_FORMAT_R8G8B8_UNORM
    };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)C4_COUNTOF(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
    #ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
    #else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
    #endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], C4_COUNTOF(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    C4_CHECK(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
}


void CleanupVulkanWindow()
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}


C4_CONST bool NeedsSwapchainRebuild(VkResult err) noexcept
{
    return (err == VK_ERROR_OUT_OF_DATE_KHR ||
            err == VK_ERROR_SURFACE_LOST_KHR ||
            err == VK_SUBOPTIMAL_KHR);
}

/** @return true if the swap chain should be rebuilt */
bool FrameStart(ImGui_ImplVulkanH_Window *wd)
{
    //! @todo the semaphores were lightly refactored from the original
    //! imgui demo implementation, and are (or are suspected to be)
    //! creating problems in some scenarios. Investigate and fix.
    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;

    // this call will bump FrameIndex
    const uint64_t timeout_ns = UINT64_MAX;//UINT64_C(60'000'000);
    const uint32_t prevFrameIndex = wd->FrameIndex;
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, timeout_ns, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    bool needs_rebuild = false;
    if(NeedsSwapchainRebuild(err))
        needs_rebuild = true;
    else if(err != VK_TIMEOUT)
        C4_CHECK_VK(err);
    else
    {
        QUICKGUI_LOGF("FrameStart(): timeout!");
        if(wd->FrameIndex == prevFrameIndex)
            return needs_rebuild;
    }

    C4_ASSERT(wd->FrameIndex < wd->ImageCount);
    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    if(err != VK_TIMEOUT)
    {
        // wait indefinitely instead of periodically checking
        C4_CHECK_VK(vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX));
    }
    C4_CHECK_VK(vkResetFences(g_Device, 1, &fd->Fence));

    {
        C4_CHECK_VK(vkResetCommandPool(g_Device, fd->CommandPool, 0));
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        C4_CHECK_VK(vkBeginCommandBuffer(fd->CommandBuffer, &info));
        C4_CHECK_VK(vkBeginCommandBuffer(fd->CommandBuffer2, &info));
        fd->CommandBuffer2Used = false;
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = (uint32_t)wd->Width;
        info.renderArea.extent.height = (uint32_t)wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    return needs_rebuild;
}


void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];

    // Recpfstdord dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);

    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;
        if(fd->CommandBuffer2Used)
        {
            ++info.commandBufferCount;
            fd->CommandBuffer2Used = false;
            C4_ASSERT(info.pCommandBuffers + 1 == &fd->CommandBuffer2);
            C4_CHECK_VK(vkEndCommandBuffer(fd->CommandBuffer2));
        }
        C4_CHECK_VK(vkEndCommandBuffer(fd->CommandBuffer));
        C4_CHECK_VK(vkQueueSubmit(g_Queue, 1, &info, fd->Fence));
    }
}


void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if(g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if(NeedsSwapchainRebuild(err))
    {
        g_SwapChainRebuild = true;
        return;
    }
    C4_CHECK_VK(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
    if(quickgui::rhi::g_rhi.m_upload_buffer_in_use)
        quickgui::rhi::g_rhi.m_upload_buffer_in_use = false;
}


C4_SUPPRESS_WARNING_MSVC_POP
C4_SUPPRESS_WARNING_GCC_CLANG_POP


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace quickgui {
namespace rhi {

c4::csubstr vk_result_to_string(VkResult err)
{
    switch(err)
    {
    #define _(n) case VK_##n: return c4::csubstr(#n);
    _(SUCCESS);
    _(NOT_READY);
    _(TIMEOUT);
    _(EVENT_SET);
    _(EVENT_RESET);
    _(INCOMPLETE);
    _(ERROR_OUT_OF_HOST_MEMORY);
    _(ERROR_OUT_OF_DEVICE_MEMORY);
    _(ERROR_INITIALIZATION_FAILED);
    _(ERROR_DEVICE_LOST);
    _(ERROR_MEMORY_MAP_FAILED);
    _(ERROR_LAYER_NOT_PRESENT);
    _(ERROR_EXTENSION_NOT_PRESENT);
    _(ERROR_FEATURE_NOT_PRESENT);
    _(ERROR_INCOMPATIBLE_DRIVER);
    _(ERROR_TOO_MANY_OBJECTS);
    _(ERROR_FORMAT_NOT_SUPPORTED);
    _(ERROR_FRAGMENTED_POOL);
    //_(ERROR_UNKNOWN); //
    _(ERROR_OUT_OF_POOL_MEMORY);
    _(ERROR_INVALID_EXTERNAL_HANDLE);
    //_(ERROR_FRAGMENTATION);
    //_(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
    //_(PIPELINE_COMPILE_REQUIRED);
    _(ERROR_SURFACE_LOST_KHR);
    _(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    _(SUBOPTIMAL_KHR);
    _(ERROR_OUT_OF_DATE_KHR);
    _(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    _(ERROR_VALIDATION_FAILED_EXT);
    _(ERROR_INVALID_SHADER_NV);
    _(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
    //_(ERROR_NOT_PERMITTED_KHR);
    _(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
    //_(THREAD_IDLE_KHR);
    //_(THREAD_DONE_KHR);
    //_(OPERATION_DEFERRED_KHR);
    //_(OPERATION_NOT_DEFERRED_KHR);
    //_(ERROR_OUT_OF_POOL_MEMORY_KHR);
    //_(ERROR_INVALID_EXTERNAL_HANDLE_KHR);
    //_(ERROR_FRAGMENTATION_EXT);
    //_(ERROR_NOT_PERMITTED_EXT);
    //_(ERROR_INVALID_DEVICE_ADDRESS_EXT);
    //_(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR);
    //_(PIPELINE_COMPILE_REQUIRED_EXT);
    //_(ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
    //_(RESULT_MAX_ENUM = );
    #undef _n
    default:
        return c4::csubstr("unknown");
    }
}

void check_vk_result(VkResult err)
{
    if(C4_LIKELY(err == 0))
        return;
    QUICKGUI_LOGF("[vulkan] Error: VkResult={}: {}\n", err, vk_result_to_string(err));
    if(err < 0)
        C4_ERROR("[vulkan] error is fatal. aborting: %s", vk_result_to_string(err).str);
}


uint32_t vk_mem_type(VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(g_PhysicalDevice, &prop);
    for(uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if((prop.memoryTypes[i].propertyFlags & properties) == properties && (type_bits & (1u << i)))
            return i;
    return UINT32_C(0xff'ff'ff'ff); // Unable to find memoryType
}

uint32_t vk_num_bytes_per_pixel(VkFormat f)
{
    // TODO ASSERT format is not compressed, not block
    switch(f)
    {
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8_USCALED:
            return UINT32_C(1);
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8_USCALED:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_UNORM:
        case VK_FORMAT_R16_USCALED:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_SFLOAT:
            return UINT32_C(2);
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8_USCALED:
            return UINT32_C(3);
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R8G8B8A8_USCALED:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_UNORM:
        case VK_FORMAT_R16G16_USCALED:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            return UINT32_C(4);
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_UNORM:
        case VK_FORMAT_R16G16B16_USCALED:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_SFLOAT:
            return UINT32_C(6);
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_UNORM:
        case VK_FORMAT_R16G16B16A16_USCALED:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return UINT32_C(8);
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            return UINT32_C(12);
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return UINT32_C(16);
        default:
            C4_NOT_IMPLEMENTED();
    }
    return {};
}


void enable_vk_debug(bool yes)
{
#ifndef QUICKGUI_ENABLE_VULKAN_DEBUG
    QUICKGUI_LOGF("[vulkan]: cannot enable/disable debug: disabled at compile-time");
    (void)yes;
#else
    QUICKGUI_LOGF("[vulkan]: {} debug mode", c4::to_csubstr(yes ? "enabling" : "disabling"));
    if(yes)
    {
        auto pfn_vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        if(!pfn_vkCreateDebugReportCallback)
        {
            QUICKGUI_LOGF("[vulkan]: could not obtain debug function");
            return; // do not crash the application, this should be non fatal
        }
        VkDebugReportCallbackCreateInfoEXT nfo = {};
        nfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        nfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
            | VK_DEBUG_REPORT_WARNING_BIT_EXT
            | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
            /*| VK_DEBUG_REPORT_INFORMATION_BIT_EXT
            | VK_DEBUG_REPORT_DEBUG_BIT_EXT*/
            ;
        nfo.pfnCallback = debug_report_vk;
        nfo.pUserData = nullptr;
        C4_CHECK_VK(pfn_vkCreateDebugReportCallback(g_Instance, &nfo, g_Allocator, &g_DebugReport));
    }
    else
    {
        auto pfn_vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
        if(!pfn_vkDestroyDebugReportCallback)
        {
            QUICKGUI_LOGF("[vulkan]: could not obtain debug function");
            return; // do not crash the application, this should be non fatal
        }
        pfn_vkDestroyDebugReportCallback(g_Instance, g_DebugReport, g_Allocator);
    }
#endif
}


// https://www.saschawillems.de/blog/2016/05/28/tutorial-on-using-vulkans-vk_ext_debug_marker-with-renderdoc/
PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = nullptr;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = nullptr;
PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = nullptr;
PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = nullptr;
PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = nullptr;

void debug_marker_setup(VkDevice device)
{
    vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
    vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
    vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
    vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
    vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
}

void debug_marker_set_name(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
{
	if(!vkDebugMarkerSetObjectName)
        return;
    VkDebugMarkerObjectNameInfoEXT nfo = {};
    nfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    nfo.objectType = objectType;
    nfo.object = object;
    nfo.pObjectName = name;
    vkDebugMarkerSetObjectName(device, &nfo);
}

void debug_marker_mark(VkCommandBuffer cmd, const char *name, fcolor const& color)
{
    if(!vkCmdDebugMarkerInsert)
        return;
    VkDebugMarkerMarkerInfoEXT nfo = {};
    nfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    memcpy(nfo.color, &color, sizeof(fcolor));
    nfo.pMarkerName = name;
    vkCmdDebugMarkerInsert(cmd, &nfo);
}

void debug_marker_region_begin(VkCommandBuffer cmd, const char *name, fcolor const& color)
{
    if(!vkCmdDebugMarkerBegin)
        return;
    VkDebugMarkerMarkerInfoEXT nfo = {};
    nfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
    memcpy(nfo.color, &color, sizeof(fcolor));
    nfo.pMarkerName = name;
    vkCmdDebugMarkerBegin(cmd, &nfo);
}

void debug_marker_region_end(VkCommandBuffer cmd)
{
    if(!vkCmdDebugMarkerEnd)
        return;
    vkCmdDebugMarkerEnd(cmd);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

fence_id FenceCollection::reset(fence_id id, VkFenceCreateInfo const& info, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    if(!id)
        id = add_handle();
    VkFence &handle = get_handle(id);
    C4_DESTROY_VK(vkDestroyFence, dev, handle, alloc);
    C4_CHECK_VK(vkCreateFence(dev, &info, alloc, &handle));
    return id;
}

void FenceCollection::destroy(fence_id id, VkDevice v, VkAllocationCallbacks const* a)
{
    handle_type &handle = get_handle(id);
    C4_DESTROY_VK(vkDestroyFence, v, handle, a);
}

void FenceCollection::destroy_all(VkDevice v, VkAllocationCallbacks const* a)
{
    for(auto &fence : handles)
        C4_DESTROY_VK(vkDestroyFence, v, fence, a);
    handles.clear();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

buffer_id BufferCollection::reset(buffer_id id, VkBufferCreateInfo const& nfo, uint32_t mem_type, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    if(!id)
        id = add_handle();
    auto &buf = get_handle(id);
    buf.destroy(dev, alloc);
    buf.create(nfo, mem_type, dev, alloc);
    return id;
}

void BufferCollection::destroy(buffer_id id, VkDevice v, VkAllocationCallbacks const* a)
{
    get_handle(id).destroy(v, a);
}

void BufferCollection::destroy_all(VkDevice v, VkAllocationCallbacks const* a)
{
    for(auto &buf : handles)
        buf.destroy(v, a);
    handles.clear();
}


void Buffer::create(VkBufferCreateInfo const& nfo, uint32_t mem_type_bits, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    size = nfo.size;
    C4_CHECK_VK(vkCreateBuffer(dev, &nfo, alloc, &handle));
    VkMemoryRequirements req = {};
    vkGetBufferMemoryRequirements(dev, handle, &req);
    alignment = req.alignment;
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = vk_mem_type(mem_type_bits, req.memoryTypeBits);
    C4_CHECK_VK(vkAllocateMemory(dev, &alloc_info, alloc, &mem));
    C4_CHECK_VK(vkBindBufferMemory(dev, handle, mem, 0));
}

void Buffer::destroy(VkDevice v, VkAllocationCallbacks const* a)
{
    C4_DESTROY_VK(vkDestroyBuffer, v, handle, a);
    C4_DESTROY_MEM_VK(v, mem, a);
    size = {};
    alignment = {};
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

image_id ImageCollection::reset(image_id id, VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_bits, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    if(!id)
        id = add_handle();
    auto &img = get_handle(id);
    img.destroy(dev, alloc);
    img.create(nfo, mem_bits, dev, alloc);
    return id;
}

void ImageCollection::destroy(image_id id, VkDevice v, VkAllocationCallbacks const* a)
{
    get_handle(id).destroy(v, a);
}

void ImageCollection::destroy_all(VkDevice v, VkAllocationCallbacks const* a)
{
    for(auto &image : handles)
        image.destroy(v, a);
    handles.clear();
}


void Image::create(ImageLayout const& C4_RESTRICT layout_, uint32_t mem_bits, VkDevice v, VkAllocationCallbacks const* a)
{
    VkImageCreateInfo nfo = layout_.to_vk();
    layout = layout_;
    create(nfo, mem_bits, v, a);
}

void Image::create(VkImageCreateInfo const& C4_RESTRICT nfo, uint32_t mem_bits, VkDevice v, VkAllocationCallbacks const* a)
{
    C4_CHECK_VK(vkCreateImage(v, &nfo, a, &handle));
    layout = ImageLayout::make(nfo);
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(v, handle, &req);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = vk_mem_type(mem_bits, req.memoryTypeBits);
    C4_CHECK_VK(vkAllocateMemory(v, &alloc_info, a, &mem));
    C4_CHECK_VK(vkBindImageMemory(v, handle, mem, 0));
}

void Image::destroy(VkDevice v, VkAllocationCallbacks const* a)
{
    C4_DESTROY_VK(vkDestroyImage, v, handle, a);
    C4_DESTROY_MEM_VK(v, mem, a);
    layout.clear();
}

void Image::destroy_mem(VkDevice v, VkAllocationCallbacks const* a)
{
    C4_DESTROY_MEM_VK(v, mem, a);
}

//-----------------------------------------------------------------------------

void copy_buffer_to_image(VkCommandBuffer cmd, VkBuffer buf, VkImage img, VkBufferImageCopy const& C4_RESTRICT region)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img;
    barrier.subresourceRange.aspectMask = region.imageSubresource.aspectMask;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkCmdCopyBufferToImage(cmd, buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

image_view_id ImageViewCollection::reset(image_view_id id, Image const& img, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    if(!id)
        id = add_handle();
    VkImageView &imgview = get_handle(id);
    C4_DESTROY_VK(vkDestroyImageView, dev, imgview, alloc);
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = img.handle;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = img.layout.format;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.layerCount = 1;
    C4_CHECK_VK(vkCreateImageView(dev, &info, alloc, &imgview));
    return id;
}

void ImageViewCollection::destroy(image_view_id id, VkDevice v, VkAllocationCallbacks const* a)
{
    C4_DESTROY_VK(vkDestroyImageView, v, get_handle(id), a);
}

void ImageViewCollection::destroy_all(VkDevice v, VkAllocationCallbacks const* a)
{
    for(auto &imgview: handles)
        C4_DESTROY_VK(vkDestroyImageView, v, imgview, a);
    handles.clear();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

sampler_id SamplerCollection::reset(sampler_id id, VkSamplerCreateInfo const& info, VkDevice dev, VkAllocationCallbacks const* alloc)
{
    if(!id)
        id = add_handle();
    VkSampler &handle = get_handle(id);
    C4_DESTROY_VK(vkDestroySampler, dev, handle, alloc);
    C4_CHECK_VK(vkCreateSampler(dev, &info, alloc, &handle));
    return id;
}

void SamplerCollection::destroy(sampler_id id, VkDevice v, VkAllocationCallbacks const* a)
{
    C4_DESTROY_VK(vkDestroySampler, v, get_handle(id), a);
}

void SamplerCollection::destroy_all(VkDevice v, VkAllocationCallbacks const* a)
{
    for(auto &sampler : handles)
        C4_DESTROY_VK(vkDestroySampler, v, sampler, a);
    handles.clear();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void UploadBuffer::destroy(Rhi &rhi)
{
    m_buf.destroy(rhi.m_device, rhi.m_allocator);
    m_pos = 0;
}
void UploadBuffer::destroy()
{
    destroy(g_rhi);
}

VkDeviceSize UploadBuffer::require(Rhi &rhi, VkDeviceSize num_bytes)
{
    VkDeviceSize actual_size = rhi.required_buffer_size(num_bytes, VkDeviceSize(1));
    C4_ASSERT(actual_size >= num_bytes);
    m_pos = 0;
    if(actual_size > m_buf.size)
    {
        // free the existing buffer before allocating with the new size
        m_buf.destroy(rhi.m_device, rhi.m_allocator);
        // create the buffer with the required size
        VkBufferCreateInfo nfo = {};
        nfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        nfo.size = actual_size;
        nfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        nfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        m_buf.create(nfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, rhi.m_device, rhi.m_allocator);
        debug_marker_set_name(rhi.m_device, m_buf.mem, "rhi_upload_buffer_memory");
        debug_marker_set_name(rhi.m_device, m_buf.handle, "rhi_upload_buffer");
        return m_buf.size;
    }
    return actual_size;
}

VkDeviceSize UploadBuffer::add(Rhi &rhi, void const *mem, VkDeviceSize sz, VkDeviceSize texelSz)
{
    VkDeviceSize mappedSize = rhi.required_buffer_size(sz, texelSz); // sz must be a multiple
    VkDeviceSize offset = next_multiple(m_pos, texelSz);
    //printf("--- add: sz=%zu->%zu pos=%zu->%zu!\n", sz, mappedSize, m_pos, offset);
    C4_CHECK(offset + mappedSize <= m_buf.size);
    void* map = nullptr;
    C4_CHECK_VK(vkMapMemory(rhi.m_device, m_buf.mem, offset, mappedSize, /*flags*/0, &map));
    memcpy(map, mem, sz);
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = m_buf.mem;
    range.offset = offset;
    range.size = mappedSize;
    C4_CHECK_VK(vkFlushMappedMemoryRanges(rhi.m_device, 1, &range));
    vkUnmapMemory(rhi.m_device, m_buf.mem);
    m_pos = offset + mappedSize;
    //printf("--- add ok!\n");
    return offset;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// piggyback on the quick'n'dirty imgui implementation
Rhi::Rhi() : Rhi(g_Device, g_PhysicalDevice, g_Allocator)
{
}

Rhi::Rhi(VkDevice device, VkPhysicalDevice phys_device, VkAllocationCallbacks const* allocator)
    : m_device(device)
    , m_phys_device(phys_device)
    , m_allocator(allocator)
    , m_fences()
    , m_image_views()
    , m_samplers()
    , m_images()
    , m_buffers()
    , m_non_coherent_atom_size(64)
    , m_upload_buffer()
    , m_upload_buffer_in_use(false)
{
    if(m_phys_device != VK_NULL_HANDLE)
    {
        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(m_phys_device, &props);
        m_non_coherent_atom_size = props.limits.nonCoherentAtomSize; // typically 64B
    }
}

Rhi::~Rhi()
{
    VkDevice v = m_device;
    VkAllocationCallbacks const* a = m_allocator;
    m_buffers.destroy_all(v, a);
    m_images.destroy_all(v, a);
    m_samplers.destroy_all(v, a);
    m_image_views.destroy_all(v, a);
    m_fences.destroy_all(v, a);
    m_upload_buffer.destroy(*this);
}

VkCommandBuffer Rhi::usr_cmd_buffer()
{
    // FIXME
    return g_MainWindowData.Frames[g_MainWindowData.FrameIndex].CommandBuffer2;
}

void Rhi::mark_usr_cmd_buffer()
{
    g_MainWindowData.Frames[g_MainWindowData.FrameIndex].CommandBuffer2Used = true;
}

VkDeviceSize Rhi::required_buffer_size(VkDeviceSize wanted, VkDeviceSize texelSize) const
{
    C4_ASSERT(m_non_coherent_atom_size > 0);
    const VkDeviceSize lcm = std::lcm(m_non_coherent_atom_size, texelSize);
    const VkDeviceSize result = next_multiple(wanted, lcm);
    printf("wanted=%lu(0x%x) noncoherentatomsize=%zu(0x%x) result=%zu(0x%x) rem=%zu(0x%xu)\n",
           wanted, (unsigned)wanted,
           m_non_coherent_atom_size, (unsigned)m_non_coherent_atom_size,
           result, (unsigned)result,
           result % m_non_coherent_atom_size, (unsigned)(result % m_non_coherent_atom_size)
        );
    C4_ASSERT(result >= wanted);
    C4_ASSERT((result % m_non_coherent_atom_size) == VkDeviceSize(0));
    C4_ASSERT((result % texelSize) == VkDeviceSize(0));
    return result;
}

size_t Rhi::use_upload_buffer_with(size_t wanted_upload_size)
{
    C4_CHECK(!m_upload_buffer_in_use);
    m_upload_buffer_in_use = true;
    return m_upload_buffer.require(*this, wanted_upload_size);
}

void Rhi::upload_image(image_id id, ImageLayout const& layout, ccharspan data, VkCommandBuffer cmdbuf)
{
    // ensure the staging upload buffer has enough room
    size_t num_bytes = layout.num_bytes();
    (void)use_upload_buffer_with(num_bytes);
    // copy to the staging upload buffer
    VkDeviceSize offset = m_upload_buffer.add(*this, data.data(), data.size(), (VkDeviceSize)layout.num_bytes_per_pixel()); // this ensures adequate room
    // now upload
    upload_image(id, layout, data, cmdbuf, &m_upload_buffer, offset);
}

void Rhi::upload_image(image_id id, ImageLayout const& layout, ccharspan data, VkCommandBuffer cmdbuf, UploadBuffer *upload_buffer, VkDeviceSize upload_buffer_offset)
{
    auto &img = get_image(id);
    C4_ASSERT(data.size() == layout.num_bytes());C4_UNUSED(data);
    // copy the staging buffer to the image, ensuring synchronization
    VkBufferImageCopy region = {};
    // see https://stackoverflow.com/questions/46501832/vulkan-vkbufferimagecopy-for-partial-transfer
    region.bufferOffset = upload_buffer_offset;
    region.bufferRowLength = layout.width;
    region.bufferImageHeight = layout.height;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1; // FIXME
    region.imageExtent.width = img.layout.width;
    region.imageExtent.height = img.layout.height;
    region.imageExtent.depth = img.layout.depth;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = img.handle;
    barrier.subresourceRange.aspectMask = region.imageSubresource.aspectMask;
    barrier.subresourceRange.levelCount = 1; // FIXME
    barrier.subresourceRange.layerCount = 1; // FIXME
    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkCmdCopyBufferToImage(cmdbuf, upload_buffer->m_buf, img.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}


//-----------------------------------------------------------------------------

void ImageDynamicCpu2Gpu::set_name(Rhi *rhi_, const char *name)
{
    for(uint32_t idx : irange(num_entries))
        rhi_->set_name(img_ids[idx], name);
}

void ImageDynamicCpu2Gpu::reset(Rhi *rhi_, VkImageCreateInfo const& C4_RESTRICT info)
{
    // For b:
    //
    //     b1. Transition b to GENERAL
    //     b2. Synchronize to CPU (likely waiting on VkFence or vkQueueIdle)
    //     b3. invalidate (if non-coherent), write it, flush (if non-coherent)
    //     b4. Synchronize to GPU (done implicitly if 3. before queue submission)
    //     b5. Transition b to TRANSFER
    //     b6. Synchronize to make sure i is not in use (likely waiting on a VkSemaphore)
    //     b7. Transition i to TRANSFER
    //     b8. Do vkCmdCopy* from b to i
    //     b9. Synchronize to make known I am finished with i (likely signalling a VkSemaphore)
    //     b0. start over at b1.
    //
    // (The fence at 2. and semaphore at 6. have to be pre-signalled or skipped for first frame to work)
    //
    // For i:
    //
    //     1. Synchronize to make sure i is free to use (likely waiting on a VkSemaphore)
    //     2. Transition i to whatever needed
    //     3. Do your rendering
    //     4. Synchronize to make known I am finished with i (likely signalling a VkSemaphore)
    //     0. start over at 1.
//static_assert(num_entries > 1);
    rhi = rhi_;
    rgpu = 0;
    wcpu = (rgpu + 1) % num_entries;
    const uint32_t nbpp = vk_num_bytes_per_pixel(info.format);
    const uint32_t num_bytes = nbpp * info.extent.width * info.extent.height * info.extent.depth * info.arrayLayers;
    VkDevice v = rhi->m_device;
    for(uint32_t idx : irange(num_entries))
    {
        // create VkImage i with UNDEFINED layout backed by DEVICE_LOCAL memory
        img_ids[idx] = rhi->reset_image(img_ids[idx], info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        // create VkBuffer b with UNDEFINED backed by HOST_VISIBLE memory
        VkBufferCreateInfo bnfo = {};
        bnfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bnfo.size = num_bytes;
        bnfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bnfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buf_ids[idx] = rhi->reset_buffer(buf_ids[idx], bnfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        // map VkBuffer b permanently
        void* mapped = nullptr;
        C4_CHECK_VK(vkMapMemory(v, buf(idx).mem, 0, num_bytes, 0, &mapped));
        bufmem[idx] = {(char*)mapped, num_bytes};
        // prepare synchronization primitives between i and b: E.g. two Semaphores, or Events could be used or Barriers if the transfer is in the same queue
        VkFenceCreateInfo fnfo = {};
        fnfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fnfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fence_ids[idx] = rhi->reset_fence(fence_ids[idx], fnfo);
    }
}

charspan ImageDynamicCpu2Gpu::start_wcpu()
{
    ImageLayout const& layout_ = layout();
    VkOffset3D first = {};
    VkOffset3D last = {0, c4::szconv<int32_t>(layout_.height), 0};
    return start_wcpu(first, last);
}

void ImageDynamicCpu2Gpu::finish_wcpu(ccharspan written)
{
    ImageLayout const& layout_ = layout();
    VkOffset3D first = {};
    VkOffset3D last = {0, c4::szconv<int32_t>(layout_.height), 0};
    finish_wcpu(first, last, written);
}

charspan ImageDynamicCpu2Gpu::start_wcpu(VkOffset3D first, VkOffset3D last)
{
    VkDevice v = rhi->m_device;
    VkCommandBuffer cmd = rhi->usr_cmd_buffer();
    rhi->mark_usr_cmd_buffer();
    VkExtent3D img_xt = img(wcpu).layout.extent();
    VkDeviceSize px_offs = c4::szconv<VkDeviceSize>(region_offset(first, img_xt));
    VkDeviceSize px_size = c4::szconv<VkDeviceSize>(region_offset(last, img_xt)) - px_offs;
    VkDeviceSize nbpp = vk_num_bytes_per_pixel(img(wcpu).layout.format);
    VkDeviceSize byte_offs = px_offs * nbpp;
    VkDeviceSize byte_size = px_size * nbpp;
    // b1. Transition b to GENERAL
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buf(wcpu).handle;
    barrier.offset = byte_offs;
    barrier.size = byte_size;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    // b2. Synchronize to CPU (likely waiting on VkFence or vkQueueIdle)
    C4_UNUSED(v);
    // TODO
    return bufmem[wcpu].subspan(byte_offs, byte_size);
}

void ImageDynamicCpu2Gpu::finish_wcpu(VkOffset3D first, VkOffset3D last, ccharspan written)
{
    C4_ASSERT(last.x >= first.x);
    C4_ASSERT(last.y >= first.y);
    C4_ASSERT(last.z >= first.z);
    auto &curr_buf = buf(wcpu);
    auto &curr_img = img(wcpu);
    VkDevice v = rhi->m_device;
    VkCommandBuffer cmd = rhi->usr_cmd_buffer();
    VkExtent3D img_xt = curr_img.layout.extent();
    VkExtent3D reg_xt = region_extent(first, last);
    VkDeviceSize nbpp = curr_img.layout.num_bytes_per_pixel();
    VkDeviceSize offset = nbpp * c4::szconv<VkDeviceSize>(region_offset(first, img_xt));
    C4_ASSERT(written.begin() >= bufmem[wcpu].begin());
    C4_ASSERT(written.end()   <= bufmem[wcpu].end());
    C4_ASSERT(written.begin() >= bufmem[wcpu].begin() + (size_t)region_offset(first, img_xt) * nbpp);
    C4_ASSERT(written.end()   <= bufmem[wcpu].begin() + (size_t)region_offset(last, img_xt) * nbpp);
    C4_ASSERT(written.begin() >= bufmem[wcpu].begin() + offset);
    C4_ASSERT(written.end()   <= bufmem[wcpu].begin() + offset + nbpp * c4::szconv<VkDeviceSize>(region_size(first, last, img_xt)));
    // b3. flush
    VkMappedMemoryRange mmr = {};
    mmr.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mmr.memory = curr_buf.mem;
    mmr.offset = offset; // must be a multiple of nonCoherentAtomSize
    mmr.offset = prev_multiple(mmr.offset, rhi->m_non_coherent_atom_size);
    C4_ASSERT(offset >= mmr.offset);
    mmr.size = written.size() + offset - mmr.offset; // must be a multiple of nonCoherentAtomSize
    mmr.size = next_multiple(mmr.size, rhi->m_non_coherent_atom_size);
    mmr.size = min(mmr.offset + mmr.size, bufmem[wcpu].size()) - mmr.offset;
    C4_CHECK_VK(vkFlushMappedMemoryRanges(v, 1, &mmr));
    // b4. Synchronize to GPU (done implicitly if 3. before queue submission)
    // TODO
    // b5. Transition b to TRANSFER
    VkBufferMemoryBarrier bbarrier = {};
    bbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bbarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    bbarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    bbarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bbarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    bbarrier.buffer = curr_buf.handle;
    bbarrier.offset = offset;
    bbarrier.size = written.size();
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bbarrier, 0, nullptr);
    // b6. Synchronize to make sure i is not in use
    // TODO
    // b7. Transition i to TRANSFER
    VkImageMemoryBarrier ibarrier = {};
    ibarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ibarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ibarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ibarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ibarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ibarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    ibarrier.image = curr_img.handle;
    ibarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ibarrier.subresourceRange.levelCount = 1;
    ibarrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ibarrier);
    // b8. Do vkCmdCopy* from b to i
    VkBufferImageCopy region = {};
    region.bufferOffset = offset;
    region.bufferRowLength = img_xt.width;
    region.bufferImageHeight = img_xt.height;
    region.imageOffset = first;
    region.imageExtent = reg_xt;
    region.imageExtent.depth = 1;   // FIXME
    region.imageExtent.width = curr_img.layout.width;  // FIXME
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    vkCmdCopyBufferToImage(cmd, curr_buf.handle, curr_img.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    // b9. Synchronize to make known i is ready
    ibarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ibarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    ibarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ibarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ibarrier);
}


} // namespace rhi
} // namespace quickgui

C4_SUPPRESS_WARNING_GCC_CLANG_POP
