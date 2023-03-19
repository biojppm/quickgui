cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(./ext/c4core/cmake/c4Project.cmake)
project(quickgui
    DESCRIPTION "quick gui"
    LANGUAGES CXX C)
c4_project(VERSION 0.0.1
    CXX_STANDARD 17
    AUTHOR "joaoppmagalhaes@gmail.com")


#-------------------------------------------------------
option(QUICKGUI_VULKAN_DEBUG "use the vulkan debug layers" ON)
option(QUICKGUI_USE_OPENCV "" OFF)
option(QUICKGUI_USE_FFMPEG "" OFF) # WIP
option(QUICKGUI_USE_FLATBUFFERS "" OFF) # WIP
if(QUICKGUI_USE_OPENCV OR QUICKGUI_USE_FFMPEG)
    set(QUICKGUI_VIDEO_ENABLED ON)
else()
    set(QUICKGUI_VIDEO_ENABLED OFF)
endif()


#-------------------------------------------------------
# dependencies

c4_require_subproject(c4core SUBDIRECTORY ${QUICKGUI_EXT_DIR}/c4core SET_FOLDER_TARGETS ext c4core c4core-amalgamate)
c4_require_subproject(c4fs SUBDIRECTORY ${QUICKGUI_EXT_DIR}/c4fs SET_FOLDER_TARGETS ext c4fs)
include(./ext/sdl.cmake)
include(./ext/imgui.cmake)
find_package(Vulkan REQUIRED)
if(QUICKGUI_USE_OPENCV)
    include(./ext/opencv.cmake)
endif()
if(QUICKGUI_USE_FLATBUFFERS)
    include(./ext/flatbuffers.cmake)
endif()


#-------------------------------------------------------

c4_add_library(quickgui
    SOURCES
        src/quickgui/color.hpp
        src/quickgui/gui.cpp
        src/quickgui/gui.hpp
        src/quickgui/imgui.hpp
        src/quickgui/imgui_impl_sdl.cpp
        src/quickgui/imgui_impl_sdl.h
        src/quickgui/imgui_impl_vulkan.cpp
        src/quickgui/imgui_impl_vulkan.h
        src/quickgui/imgview.cpp
        src/quickgui/imgview.hpp
        src/quickgui/log.cpp
        src/quickgui/log.hpp
        src/quickgui/math.hpp
        src/quickgui/mem.hpp
        src/quickgui/overlay_canvas.cpp
        src/quickgui/overlay_canvas.hpp
        src/quickgui/primitive_draw_list.hpp
        src/quickgui/primitive_draw_list_multithread.hpp
        src/quickgui/rhi.cpp
        src/quickgui/rhi.hpp
        src/quickgui/sdl.hpp
        src/quickgui/static_vector.hpp
        src/quickgui/stb_image.h
        src/quickgui/stb_image_data.cpp
        src/quickgui/stb_image_data.hpp
        src/quickgui/string.hpp
        src/quickgui/time.cpp
        src/quickgui/time.hpp
        src/quickgui/widgets.cpp
        src/quickgui/widgets.hpp
    INC_DIRS
        $<BUILD_INTERFACE:${QUICKGUI_SRC_DIR}> $<INSTALL_INTERFACE:${QUICKGUI_SRC_DIR}>
        $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}> $<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>
    PUBLIC_DEFS
        $<$<BOOL:${QUICKGUI_VULKAN_DEBUG}>:QGUI_ENABLE_VULKAN_DEBUG>
    LIBS
        c4fs
        c4core
        imgui
        ${QUICKGUI_SDL}
        ${Vulkan_LIBRARY}
        )


if(QUICKGUI_USE_OPENCV)
    c4_add_library(quickgui-cvutil
        SOURCES
            src/quickgui/cvutil/cvutil.hpp
            src/quickgui/cvutil/cvutil.cpp
        INC_DIRS
            $<BUILD_INTERFACE:${QUICKGUI_SRC_DIR}> $<INSTALL_INTERFACE:${QUICKGUI_SRC_DIR}>
        PUBLIC_DEFS
            QUICKGUI_USE_CV
        LIBS
            quickgui
            ${QUICKGUI_OPENCV_LIBRARIES}
        )
endif()


if(QUICKGUI_VIDEO_ENABLED)
    c4_add_library(quickgui-video
        SOURCES
            src/quickgui/video/video_frame.hpp
            src/quickgui/video/video_reader.hpp
            src/quickgui/video/video_reader.cpp
            src/quickgui/video/video_source.hpp
        INC_DIRS
            $<BUILD_INTERFACE:${QUICKGUI_SRC_DIR}> $<INSTALL_INTERFACE:${QUICKGUI_SRC_DIR}>
        LIBS
            quickgui
            $<$<BOOL:${QUICKGUI_USE_OPENCV}>:quickgui-cvutil>
    )
endif()


#-------------------------------------------------

if(QUICKGUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

option(QUICKGUI_BUILD_SAMPLES "" ON)
if(QUICKGUI_BUILD_SAMPLES)
    add_subdirectory(sample)
endif()
