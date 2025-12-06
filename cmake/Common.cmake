# Project-wide C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compilation database for clangd / code analysis
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include Modules for compiler flags, platform detection, and utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

include(Platform)
include(Compiler)

# Default include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# For Emscripten builds
if(DEFINED EMSCRIPTEN_ROOT_PATH)
    include_directories(${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include)
endif()

option(AX_BUILD_EMWEB "Build for WebAssembly via Emscripten" OFF)

# Utility: output compiler info
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Platform: ${AX_PLATFORM}")
if(AX_BUILD_EMWEB)
    message(STATUS "Building for WebAssembly (Emscripten)")
endif()

# Option defaults
option(AX_IMPL_ASSIMP "Enable Assimp support" ON)
option(AX_IMPL_BULLET3 "Enable Bullet3 Physics support" ON)
option(AX_IMPL_TRUETYPES "Enable FreeType (TrueType fonts) support" ON)
option(AX_IMPL_LAYER_SDL3 "Enable SDL3 Application Layer support" ON)
option(AX_IMPL_LAYER_X11 "Enable X Window Application Layer support" OFF)
option(AX_IMPL_LAYER_WIN32 "Enable Win32 Application Layer support" OFF)
option(AX_BUILD_EXAMPLES "Build Examples, Hello Window, Spinning Cube, etc." OFF)