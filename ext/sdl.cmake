option(QUICKGUI_SDL_STATIC "use the static version of the SDL library" ON)
if(QUICKGUI_SDL_STATIC)
    set(QUICKGUI_SDL_OVERRIDE SDL_STATIC ON SDL_SHARED OFF)
    set(QUICKGUI_SDL SDL2-static)
else()
    set(QUICKGUI_SDL_OVERRIDE SDL_STATIC OFF SDL_SHARED ON)
    set(QUICKGUI_SDL SDL2)
endif()

c4_require_subproject(sdl2 SUBDIRECTORY ${QUICKGUI_EXT_DIR}/SDL
    OVERRIDE
        SDL_TEST OFF
        SDL2_DISABLE_SDL2MAIN ON
        SDL2_DISABLE_UNINSTALL ON
        ${QUICKGUI_SDL_OVERRIDE}
    SET_FOLDER_TARGETS ext/SDL
        SDL2
        SDL2-static
        sdl_headers_copy
    )

c4_target_compile_flags(${QUICKGUI_SDL} PRIVATE
    GCC_CLANG
        -Wno-pedantic
    GCC
    CLANG
    MSVC
    )
