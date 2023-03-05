option(QUICKGUI_WITH_DEMOS "" ON)

c4_add_library(imgui
    SOURCES
        imgui/imgui.h
        imgui/imconfig.h
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h
        imgui/misc/debuggers/imgui.natvis
        #
        implot/implot.h
        implot/implot.cpp
        implot/implot_internal.h
        implot/implot_items.cpp
        #
        imguifilesystem/dirent_portable.h
        imguifilesystem/imguifilesystem.h
        imguifilesystem/imguifilesystem.cpp
        #
        imguispinner/imgui_spinner.h
        imguispinner/imgui_spinner.cpp
        #
        $<$<BOOL:${QUICKGUI_WITH_DEMOS}>:imgui/imgui_demo.cpp>
        $<$<BOOL:${QUICKGUI_WITH_DEMOS}>:implot/implot_demo.cpp>
        #
        imgui.cmake
    SOURCE_ROOT ${QUICKGUI_EXT_DIR}
    INC_DIRS
        ${QUICKGUI_EXT_DIR}/imgui
        ${QUICKGUI_EXT_DIR}/imgui/backends
        ${QUICKGUI_EXT_DIR}/implot
        ${QUICKGUI_EXT_DIR}/imguifilesystem
        ${QUICKGUI_EXT_DIR}/imguispinner
    DEFS
        $<$<BOOL:${QUICKGUI_WITH_DEMOS}>:QUICKGUI_WITH_DEMOS>
    FOLDER
        ext
    )

c4_target_compile_flags(imgui PRIVATE
    GCC_CLANG
        -Wno-conversion
        -Wno-sign-conversion
        -Wno-double-promotion
        -Wno-float-equal
        -Wno-format-nonliteral
        -Wno-shadow
        -Wno-unused-parameter
        -Wno-unused-result
    CLANG
        -Wno-format-pedantic
    GCC
        -Wno-useless-cast
        -Wno-stringop-truncation
        -Wno-stringop-overflow
    MSVC
        /wd4100  # 'count': unreferenced formal parameter
        /wd4127  # conditional expression is constant
        /wd4242  # '=': conversion from 'int' to 'char', possible loss of data
        /wd4244  # 'initializing': conversion from 'float' to 'int', possible loss of data
        /wd4267  # 'argument': conversion from 'size_t' to 'int', possible loss of data
        /wd4305  # 'argument': truncation from 'double' to 'float'
        /wd4456  # declaration of 'fi' hides previous local declaration
        /wd4458  # declaration of 'SplitPath' hides class member
        /wd4505  # '_wreaddir': unreferenced local function has been removed
        /wd4706  # assignment within conditional expression
        /wd4996  # This function or variable may be unsafe
    )
