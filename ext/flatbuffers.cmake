
option(QUICKGUI_FLATBUFFERS_BUILD "" ON)

if(NOT QUICKGUI_FLATBUFFERS_BUILD)
    find_library(flatbuffers REQUIRED)
    find_program(QUICKGUI_FLATC flatc REQUIRED)
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

set(FLATBUFFERS_DEFAULT_CPP_FLAGS
    --cpp --gen-mutable --gen-object-api --reflect-names
)

function(flatbuffers_compile_schema_to_cpp schema_rel)
    _c4_handle_args(_ARGS ${ARGN}
        _ARGS0
          GRPC      # enable GRPC
          NO_DEFAULT_OPTIONS # do not use FLATBUFFERS_DEFAULT_CPP_FLAGS
        _ARGS1
          API_ROOT  # common part to add to the path
          GENDIR    # path to the directory where to generate the files
          GENFILES  # variable to append the generated files
          INC_DIR   # variable to output the include directory
        _ARGSN
          FLATC_OPTIONS  # options to pass to flatc (other than FLATBUFFERS_DEFAULT_CPP_FLAGS)
    )
    message(STATUS "${schema_rel}: add generation of C++ code")
    if(NOT _API_ROOT)
        message(FATAL_ERROR "needs API_ROOT")
    endif()
    # create the full filename of the schema
    set(schema ${_API_ROOT}/${schema_rel})
    # get the intermediate path
    get_filename_component(schema_dirname ${schema_rel} PATH)
    get_filename_component(schema_basename ${schema} NAME)
    set(gen_root "${CMAKE_CURRENT_BINARY_DIR}/flatbuffers_generated")
    set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/flatbuffers_generated/${schema_dirname}")
    file(MAKE_DIRECTORY ${gen_dir})
    if(_INC_DIR)
        set(${_INC_DIR} ${gen_root} PARENT_SCOPE)
    endif()
    string(REGEX REPLACE "\\.fbs$" "_generated.h" gen_header ${gen_dir}/${schema_basename})
    set(genfiles ${gen_header})
    if(_GRPC)
        string(REGEX REPLACE "\\.fbs$" ".grpc.fb.h" gen_grpc_h ${gen_dir}/${schema_basename})
        string(REGEX REPLACE "\\.fbs$" ".grpc.fb.cc" gen_grpc_cc ${gen_dir}/${schema_basename})
        list(APPEND genfiles ${gen_grpc_h} ${gen_grpc_cc})
    endif()
    if(_GENFILES)
        set(${_GENFILES} ${${_GENFILES}} ${genfiles} PARENT_SCOPE)
    endif()
    set(opts)
    if(NOT _NO_DEFAULT_OPTIONS)
        set(opts ${FLATBUFFERS_DEFAULT_CPP_FLAGS})
    endif()
    set(opts ${opts} ${FLATC_OPTIONS})
    set(cmd ${QUICKGUI_FLATC}
                -I "${_API_ROOT}"
                -o "${gen_dir}"
                ${opts}
                $<$<BOOL:${_GRPC}>:--grpc>
                ${schema})
    add_custom_command(OUTPUT ${genfiles}
        DEPENDS ${schema}
        COMMAND echo ${cmd}
        COMMAND ${cmd}
        COMMENT "flatbuffers C++: ${schema_rel}")
endfunction()
