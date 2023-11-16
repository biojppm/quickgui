set(QUICKGUI_SDL_MAJOR 2)  # set to the major version of SDL
option(QUICKGUI_SDL_STATIC "use the static version of the SDL library" ON)

if(QUICKGUI_SDL_STATIC)
    set(QUICKGUI_SDL_OVERRIDE SDL_STATIC ON SDL_SHARED OFF)
    set(QUICKGUI_SDL SDL${QUICKGUI_SDL_MAJOR}-static)
else()
    set(QUICKGUI_SDL_OVERRIDE SDL_STATIC OFF SDL_SHARED ON)
    set(QUICKGUI_SDL SDL${QUICKGUI_SDL_MAJOR})
endif()

c4_require_subproject(sdl${QUICKGUI_SDL_MAJOR} SUBDIRECTORY ${QUICKGUI_EXT_DIR}/SDL
    OVERRIDE
        SDL_TEST OFF
        $<$<STREQUAL:${QUICKGUI_SDL_MAJOR},2>:SDL2_DISABLE_SDL2MAIN ON>
        SDL_DISABLE_UNINSTALL ON
        ${QUICKGUI_SDL_OVERRIDE}
    SET_FOLDER_TARGETS ext/SDL
        SDL${QUICKGUI_SDL_MAJOR}
        SDL${QUICKGUI_SDL_MAJOR}-static
        sdl_headers_copy
    )

c4_target_compile_flags(${QUICKGUI_SDL} PRIVATE
    GCC_CLANG
        -Wno-pedantic
    GCC
    CLANG
    MSVC
    )
