cmake_minimum_required(VERSION 3.22)

project(QuteConvert)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.0/msvc2022_64")

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets
)

qt_standard_project_setup()

add_executable(QuteConvert src/main.cpp)

target_link_libraries(QuteConvert 
    PRIVATE 
        Qt6::Core 
        Qt6::Gui 
        Qt6::Widgets
)

if (WIN32)
    add_custom_command(TARGET QuteConvert POST_BUILD
        COMMAND "C:/Qt/6.11.0/msvc2022_64/bin/windeployqt.exe"
                --no-translations
                "$<TARGET_FILE:QuteConvert>"
        COMMENT "Deploying Qt..."
    )
endif()