cmake_minimum_required(VERSION 3.24)
project(clientmod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── fetch imgui (no git submodule needed) ─────────────────────
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ── sources ───────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/hooks.cpp
    src/features.cpp
    src/menu.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

add_library(clientmod SHARED ${SOURCES})

target_include_directories(clientmod PRIVATE
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(clientmod PRIVATE d3d9 psapi)

target_compile_definitions(clientmod PRIVATE
    WIN32 _WIN32 NOMINMAX _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(clientmod PRIVATE /W3 /MP)
    # silence imgui warnings
    set_source_files_properties(
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        PROPERTIES COMPILE_FLAGS "/W0"
    )
endif()