include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(QuteConvertTests
    filediscovery_tests.cpp
    ${PROJECT_SOURCE_DIR}/src/filediscovery.cpp
)

target_include_directories(QuteConvertTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(QuteConvertTests
    PRIVATE
        Qt6::Core
        GTest::gtest_main
)

if(WIN32)
    add_custom_command(TARGET QuteConvertTests POST_BUILD
        COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
                --no-translations
                "$<TARGET_FILE:QuteConvertTests>"
        COMMENT "Deploying Qt runtime for test discovery..."
    )
endif()

include(GoogleTest)
gtest_discover_tests(QuteConvertTests)
