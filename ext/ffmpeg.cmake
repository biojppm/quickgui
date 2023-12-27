
find_package(PkgConfig REQUIRED)

set(QUICKGUI_FFMPEG_MODULES
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
    CACHE STRING "" FORCE)

add_library(quickgui-ffmpeg INTERFACE IMPORTED GLOBAL)
foreach(mod ${QUICKGUI_FFMPEG_MODULES})
    message(STATUS "${mod}")
    pkg_check_modules(QG_pkg_${mod} REQUIRED IMPORTED_TARGET ${mod})
    message(STATUS "QG_pkg_${mod}_INCLUDE_DIRS=${QG_pkg_${mod}_INCLUDE_DIRS}")
    message(STATUS "QG_pkg_${mod}_LDFLAGS=${QG_pkg_${mod}_LDFLAGS}")
    target_include_directories(quickgui-ffmpeg INTERFACE ${QG_pkg_${mod}_INCLUDE_DIRS})
    target_link_options(quickgui-ffmpeg INTERFACE ${QG_pkg_${mod}_LDFLAGS})
endforeach()
