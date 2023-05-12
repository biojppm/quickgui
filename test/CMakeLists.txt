c4_setup_testing(DOCTEST)

c4_add_executable(quickgui-test
    SOURCES
        test_main.cpp
    LIBS quickgui
)

c4_add_executable(quickgui-test-irange
    SOURCES
        test_irange.cpp
    LIBS quickgui doctest
)
