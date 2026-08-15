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

add_executable(QuteConvertWebEngineTests
  webengine_tests_main.cpp
  webengine_lifecycle_tests.cpp
  ${PROJECT_SOURCE_DIR}/src/conversioncontroller.cpp
  ${PROJECT_SOURCE_DIR}/src/filediscovery.cpp
  ${PROJECT_SOURCE_DIR}/src/htmltopdfconverter.cpp
  ${PROJECT_SOURCE_DIR}/include/quteconvert/conversioncontroller.h
  ${PROJECT_SOURCE_DIR}/include/quteconvert/conversiontypes.h
  ${PROJECT_SOURCE_DIR}/include/quteconvert/filediscovery.h
  ${PROJECT_SOURCE_DIR}/include/quteconvert/htmltopdfconverter.h
)

target_include_directories(QuteConvertWebEngineTests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(QuteConvertWebEngineTests
  PRIVATE
    QUTECONVERT_INTEGRATION_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/integration"
)

target_link_libraries(QuteConvertWebEngineTests
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    GTest::gtest
)

if(WIN32)
  add_custom_command(TARGET QuteConvertWebEngineTests POST_BUILD
    COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
            --no-translations
            "$<TARGET_FILE:QuteConvertWebEngineTests>"
    COMMENT "Deploying Qt WebEngine runtime for lifecycle tests..."
  )
endif()

set(webengine_leak_messages
  "Release of profile requested but WebEnginePage still not deleted"
  "WebFrame LEAKED"
)

gtest_discover_tests(QuteConvertWebEngineTests
  PROPERTIES
    TIMEOUT 120
    FAIL_REGULAR_EXPRESSION "${webengine_leak_messages}"
)

set(cli_integration_output
  "${CMAKE_CURRENT_BINARY_DIR}/cli-integration-output"
)
file(MAKE_DIRECTORY "${cli_integration_output}")

add_test(NAME QuteConvertCliWebEngineLifecycle
  COMMAND $<TARGET_FILE:QuteConvert>
          --input "${PROJECT_SOURCE_DIR}/tests/fixtures/integration"
          --output "${cli_integration_output}"
          --overwrite
          --settle-delay 0
)
set_tests_properties(QuteConvertCliWebEngineLifecycle PROPERTIES
  TIMEOUT 120
  FAIL_REGULAR_EXPRESSION "${webengine_leak_messages}"
)
