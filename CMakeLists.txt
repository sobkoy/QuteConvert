cmake_minimum_required(VERSION 3.22)

project(QuteConvert VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep the currently installed VS Code/Windows Qt kit as the default while still
# allowing callers to provide a different CMAKE_PREFIX_PATH.
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS
  Core
  Gui
  Widgets
  WebEngineWidgets
)

qt_standard_project_setup()

add_executable(QuteConvert
  src/main.cpp
  src/conversioncontroller.cpp
  src/filediscovery.cpp
  src/htmltopdfconverter.cpp
  src/mainwindow.cpp
  include/quteconvert/conversioncontroller.h
  include/quteconvert/conversiontypes.h
  include/quteconvert/filediscovery.h
  include/quteconvert/htmltopdfconverter.h
  include/quteconvert/mainwindow.h
)

target_include_directories(QuteConvert
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(QuteConvert
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

if(WIN32)
  add_custom_command(TARGET QuteConvert POST_BUILD
    COMMAND "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt.exe"
            --no-translations
            "$<TARGET_FILE:QuteConvert>"
    COMMENT "Deploying Qt and Qt WebEngine..."
  )
endif()

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
