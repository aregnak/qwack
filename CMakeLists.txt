cmake_minimum_required(VERSION 3.20)
project(lol_cs_counter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

# SDL
include_directories("${CMAKE_SOURCE_DIR}/libs/SDL/include")

# ImGui
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui")
include_directories("${CMAKE_SOURCE_DIR}/libs/httplib")
include_directories("${CMAKE_SOURCE_DIR}/libs/json")

# SDL lib directory (x64 build)
link_directories("${CMAKE_SOURCE_DIR}/libs/SDL/lib")

add_executable(lol_cs_counter
    src/main.cpp
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_impl_sdl2.cpp
    libs/imgui/imgui_impl_dx11.cpp
)

target_link_libraries(lol_cs_counter PRIVATE
    SDL2.lib
    SDL2main.lib
    d3d11.lib
    dxgi.lib
    OpenSSL::SSL
    OpenSSL::Crypto
)
