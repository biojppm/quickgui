
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
          hello_video/video_player_console.hpp
          hello_video/video_player_gui.hpp
          hello_video/video_reader.hpp
        LIBS
          quickgui-video
    )
endif()
