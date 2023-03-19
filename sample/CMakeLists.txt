
c4_add_executable(quickgui-sample-hello_world
    SOURCES
      hello_world/main.cpp
    LIBS
      quickgui
)

if(QUICKGUI_VIDEO_ENABLED)
    c4_add_executable(quickgui-sample-hello_video
        SOURCES
          hello_video/main.cpp
          hello_video/sample_video_player.hpp
          hello_video/sample_video_reader.hpp
        LIBS
          quickgui-video
    )
endif()

if(QUICKGUI_USE_FLATBUFFERS)
    add_subdirectory(hello_remote)
endif()
