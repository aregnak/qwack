cmake_minimum_required(VERSION 3.20)
project(qwack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

# SDL
include_directories("${CMAKE_SOURCE_DIR}/libs/SDL3/include")

# ImGui
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui")
include_directories("${CMAKE_SOURCE_DIR}/libs/httplib")
include_directories("${CMAKE_SOURCE_DIR}/libs/json")

# SDL lib directory (x64 build)
link_directories("${CMAKE_SOURCE_DIR}/libs/SDL3/lib")

add_executable(qwack
    src/main.cpp
    src/poll.cpp
    src/gamePoller.cpp
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_impl_sdl3.cpp
    libs/imgui/imgui_impl_dx11.cpp
)

target_compile_definitions(qwack PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

target_link_libraries(qwack PRIVATE
    SDL3.lib
    d3d11.lib
    dxgi.lib
    OpenSSL::SSL
    OpenSSL::Crypto
)
