
flatbuffers_compile_schema_to_cpp(api/schema.fbs
    GRPC
    INC_DIR schema_include
    API_ROOT ${CMAKE_CURRENT_SOURCE_DIR}
    GENFILES flatbufferfiles
)

find_package(gRPC REQUIRED)
c4_add_library(quickgui-sample-hello_remote-api
    SOURCES
      ${flatbufferfiles}
    LIBS
      flatbuffers
      gRPC::grpc++_unsecure
    INC_DIRS
      $<BUILD_INTERFACE:${schema_include}>
      )

c4_add_executable(quickgui-sample-hello_remote-client
    SOURCES
      client.hpp
      client_main.cpp
    LIBS
      quickgui
      quickgui-sample-hello_remote-api
)

c4_add_executable(quickgui-sample-hello_remote-server
    SOURCES
      server_main.cpp
    LIBS
      quickgui-sample-hello_remote-api
    INC_DIRS
      $<BUILD_INTERFACE:${schema_include}>
)

set(runsh ${CMAKE_CURRENT_BINARY_DIR}/quickgui-sample-hello_remote.sh)
file(WRITE ${runsh}
"#!/bin/bash -xe
# https://stackoverflow.com/questions/3004811/how-do-you-run-multiple-programs-in-parallel-from-a-bash-script

exedir=\$1
( \
  trap 'kill 0' SIGINT ; \
  $exedir/quickgui-sample-hello_remote-server & \
  $exedir/quickgui-sample-hello_remote-client &
  wait
)
")

add_custom_target(quickgui-sample-hello_remote
    COMMAND chmod a+x ${runsh}
    COMMAND ${runsh} $<TARGET_FILE_DIR:quickgui-sample-hello_remote-server>
)
