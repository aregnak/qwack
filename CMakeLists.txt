cmake_minimum_required(VERSION 3.21)

project(qwack VERSION 0.0.4)

# For consistent version
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Portable Windows build settings
option(PORTABLE "Link MSVC runtime statically for portable Windows binaries" OFF)

if (MSVC)
    cmake_policy(SET CMP0091 NEW)
    if (PORTABLE)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

if (PORTABLE)
    if (NOT DEFINED VCPKG_TARGET_TRIPLET OR NOT VCPKG_TARGET_TRIPLET MATCHES "-static")
        message(WARNING "PORTABLE=ON, but vcpkg triplet is not static. Please use -DVCPKG_TARGET_TRIPLET=x64-windows-static.")
    endif()
endif()

# OpenSSL
find_package(OpenSSL REQUIRED)

# SDL3
add_library(SDL3 SHARED IMPORTED)

set_target_properties(SDL3 PROPERTIES
    IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/libs/SDL3/lib/SDL3.lib"
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/SDL3/bin/SDL3.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libs/SDL3/include"
)

add_executable(qwack
    src/main.cpp
    src/poll.cpp
    src/gamePoller.cpp
    src/window.cpp
    src/overlayWindow.cpp
    src/menuWindow.cpp
    src/trayHelper.cpp

    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_impl_sdl3.cpp
    libs/imgui/imgui_impl_dx11.cpp
)

if (MSVC)
    target_link_options(qwack PRIVATE 
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
    )
endif()


# Header libs
target_include_directories(qwack PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    libs/imgui
    libs/httplib
    libs/json
)

target_compile_definitions(qwack PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# Link libraries
target_link_libraries(qwack PRIVATE
    SDL3
    d3d11
    dxgi
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Automatically copy required DLLs
add_custom_command(TARGET qwack POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:qwack>
    $<TARGET_FILE_DIR:qwack>
    COMMAND_EXPAND_LISTS
)