
option(QUICKGUI_FLATBUFFERS_BUILD "" ON)

if(NOT QUICKGUI_FLATBUFFERS_BUILD)
    c4_err("not implemented")
    find_package(flatbuffers REQUIRED)
    set(QUICKGUI_FLATC $<TARGET_FILE:flatc>)
else()
    set(QUICKGUI_FLATBUFFERS_REPO https://github.com/google/flatbuffers CACHE STRING "")
    set(QUICKGUI_FLATBUFFERS_VERSION v23.3.3 CACHE STRING "")
    set(QUICKGUI_FLATC $<TARGET_FILE:flatc>)
    c4_require_subproject(flatbuffers
        REMOTE
            GIT_REPOSITORY ${QUICKGUI_FLATBUFFERS_REPO}
            GIT_TAG ${QUICKGUI_FLATBUFFERS_VERSION}
            GIT_SHALLOW ON
        IMPORTED_DIR flatbuffersdir
        OVERRIDE
            FLATBUFFERS_BUILD_TESTS OFF
        SET_FOLDER_TARGETS ext/flatbuffers
            flatbuffers
            flatc
        )
endif()


#----------------------------------------------------
# adapted from flatbuffers' CMakeLists.txt


function(flatbuffers_compile_schema_to_cpp_opt schema OPT generated_files)
  message(STATUS "`${schema}`: add generation of C++ code with '${OPT}'")
  get_filename_component(schema_dir ${schema} PATH)
  if("${schema_dir}" STREQUAL "")
      set(schema_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  string(REGEX REPLACE "\\.fbs$" "_generated.hpp" gen_header ${schema})
  set(gen_header ${schema_dir}/${gen_header})
  add_custom_command(
    OUTPUT ${gen_header}
    COMMAND ${QUICKGUI_FLATC}
            --cpp --gen-mutable --gen-object-api --reflect-names
            ${OPT}
            -o "${schema_dir}"
            "${schema}"
    COMMENT "Run generation: '${gen_header}'")
  set(genfiles ${${generated_files}})
  list(APPEND genfiles ${gen_header})
  set(${generated_files} ${genfiles} PARENT_SCOPE)
endfunction()


function(flatbuffers_compile_schema_to_cpp schema generated_files)
  set(genfiles ${${generated_files}})
  flatbuffers_compile_schema_to_cpp_opt(${schema} "--no-includes;--gen-compare" genfiles)
  set(${generated_files} ${genfiles} PARENT_SCOPE)
endfunction()


function(flatbuffers_compile_schema_to_binary schema generated_files)
  message(STATUS "`${schema}`: add generation of binary (.bfbs) schema")
  get_filename_component(schema_dir ${schema} PATH)
  string(REGEX REPLACE "\\.fbs$" ".bfbs" gen_binary_schema ${schema})
  # For details about flags see generate_code.py
  add_custom_command(
    OUTPUT ${gen_binary_schema}
    COMMAND ${QUICKGUI_FLATC}
            -b --schema --bfbs-comments --bfbs-builtins
            --bfbs-filenames "${CMAKE_CURRENT_SOURCE_DIR}/${schema_dir}"
            -o "${schema_dir}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${schema}"
    COMMENT "Run generation: '${gen_binary_schema}'")
  set(genfiles ${${generated_files}})
  list(APPEND genfiles ${gen_binary_schema})
  set(${generated_files} ${genfiles} PARENT_SCOPE)
endfunction()


function(flatbuffers_compile_schema_to_embedded_binary schema OPT generated_files)
  message(STATUS "`${schema}`: add generation of C++ embedded binary schema code with '${OPT}'")
  get_filename_component(schema_dir ${schema} PATH)
  string(REGEX REPLACE "\\.fbs$" "_bfbs_generated.h" gen_bfbs_header ${schema})
  # For details about flags see generate_code.py
  add_custom_command(
    COMMAND ${QUICKGUI_FLATC}
          --cpp --gen-mutable --gen-object-api --reflect-names
          --cpp-ptr-type flatbuffers::unique_ptr # Used to test with C++98 STLs
          ${OPT}
          --bfbs-comments --bfbs-builtins --bfbs-gen-embed
          --bfbs-filenames ${schema_dir}
          -o "${schema_dir}"
          "${CMAKE_CURRENT_SOURCE_DIR}/${schema}"
          COMMENT "Run generation: '${gen_bfbs_header}'")
  set(genfiles ${${generated_files}})
  list(APPEND genfiles ${gen_bfbs_header})
  set(${generated_files} ${genfiles} PARENT_SCOPE)
endfunction()
