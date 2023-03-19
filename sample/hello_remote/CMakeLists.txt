flatbuffers_compile_schema_to_cpp(${CMAKE_CURRENT_SOURCE_DIR}/schema.fbs generated_header)

c4_add_executable(quickgui-sample-hello_remote
    SOURCES
      main.cpp
      ${generated_header}
    LIBS
      quickgui
      flatbuffers
)
