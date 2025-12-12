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
include_directories(${CMAKE_SOURCE_DIR}/include)

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
option(AX_IMPL_ASSIMP "Enable Assimp Support" ON)
option(AX_IMPL_BULLET3 "Enable Bullet3 Physics Support" ON)
option(AX_IMPL_TRUETYPES "Enable FreeType (TrueType fonts) Support" ON)
option(AX_IMPL_LAYER_SDL3 "Enable SDL3 Application Layer Support" ON)
option(AX_IMPL_LAYER_X11 "Enable X Window Application Layer Support" OFF)
option(AX_IMPL_LAYER_WIN32 "Enable Win32 Application Layer Support" OFF)
option(AX_IMPL_ASSETS_MODEL_LOADER_OBJ "Build Wavefront Obj Model Loader" OFF)
option(AX_IMPL_ASSETS_MODEL_LOADER_FBX "Build FBX Model Loader" OFF)
option(AX_IMPL_ASSETS_MODEL_LOADER_GLTF2 "Build glTF2 Model Loader" OFF)
option(AX_IMPL_GRAPHICS_GL "Enable OpenGL Graphics Support" OFF)
option(AX_IMPL_GRAPHICS_DX11 "Enable DirectX11 Graphics Support" OFF)
option(AX_IMPL_AUDIO_SOFTOPENAL "Enable soft-openal Audio Support" OFF)
option(AX_BUILD_EXAMPLES "Build Examples, Hello Window, Spinning Cube, etc." OFF)