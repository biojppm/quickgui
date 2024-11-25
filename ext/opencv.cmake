
# Usually opencv is obtained from an existing install.  But this
# project is deployed in some embedded platforms where that is not
# desirable. So opencv can be compiled here.

option(QUICKGUI_OPENCV_BUILD "" OFF)

if(NOT QUICKGUI_OPENCV_BUILD)
    find_package(OpenCV REQUIRED)
else()
    set(QUICKGUI_OPENCV_REPO https://github.com/opencv/opencv CACHE STRING "")
    set(QUICKGUI_OPENCV_VERSION 4.7.0 CACHE STRING "")
    # use only these modules:
    set(QUICKGUI_OPENCV_MODULES core imgproc imgcodecs)
    option(QUICKGUI_OPENCV_VIDEO "" OFF)
    if(QUICKGUI_OPENCV_VIDEO)
        list(APPEND QUICKGUI_OPENCV_MODULES video videoio)
    endif()
    set(QUICKGUI_OPENCV_MODULES_EXTRA "" CACHE STRING "")
    list(APPEND QUICKGUI_OPENCV_MODULES ${QUICKGUI_OPENCV_MODULES_EXTRA})
    set(QUICKGUI_OPENCV_MODULES_)
    set(QUICKGUI_OPENCV_LIBRARIES)
    foreach(mod ${QUICKGUI_OPENCV_MODULES})
        set(QUICKGUI_OPENCV_MODULES_ "${QUICKGUI_OPENCV_MODULES_}${mod},")
        list(APPEND QUICKGUI_OPENCV_LIBRARIES opencv_${mod})
    endforeach()
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "")
        c4_err("CMAKE_SYSTEM_PROCESSOR is not set???")
    endif()
    c4_require_subproject(opencv
        REMOTE
            GIT_REPOSITORY ${QUICKGUI_OPENCV_REPO}
            GIT_TAG ${QUICKGUI_OPENCV_VERSION}
            GIT_SHALLOW ON
        IMPORTED_DIR opencvdir
        OVERRIDE
            BUILD_LIST "${QUICKGUI_OPENCV_MODULES_}"
            WITH_FFMPEG ${QUICKGUI_USE_FFMPEG}
            WITH_OPENCL OFF
            WITH_VTK OFF
            WITH_VULKAN ON
            BUILD_TESTS OFF
            BUILD_PERF_TESTS OFF
        SET_FOLDER_TARGETS ext/opencv
            opencv_highgui_plugins
            opencv_modules
            opencv_perf_tests
            opencv_tests
            opencv_videoio_plugins
            opencv_annotation
            opencv_core
            opencv_highgui
            opencv_imgcodecs
            opencv_imgproc
            opencv_perf_core
            opencv_perf_imgcodecs
            opencv_perf_imgproc
            opencv_perf_video
            opencv_perf_videoio
            opencv_test_core
            opencv_test_highgui
            opencv_test_imgcodecs
            opencv_test_imgproc
            opencv_test_video
            opencv_test_videoio
            opencv_ts
            opencv_version
            opencv_video
            opencv_videoio
            opencv_visualisation
            )
    # opencv is usually obtained from a prior install.
    # but we're using inside our cmake build directory,
    # so there's some other things we need to do.
    target_link_libraries(opencv_core PUBLIC cblas)
    foreach(mod ${QUICKGUI_OPENCV_MODULES})
        # setup the include directories for each cv module:
        target_include_directories(opencv_${mod} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
            $<BUILD_INTERFACE:${opencvdir}/modules/${mod}/include>
            $<BUILD_INTERFACE:${opencvdir}/include>
            )
        # setup the cv module dependencies
        if(NOT (mod STREQUAL core))
            target_link_libraries(opencv_${mod} PUBLIC opencv_core)
        endif()
    endforeach()
endif()
