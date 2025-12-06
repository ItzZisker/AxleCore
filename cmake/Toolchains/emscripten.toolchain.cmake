# emscripten.toolchain.cmake
# Toolchain file for cross-compiling AxiomCore / Syngine to WebAssembly via Emscripten

# Tell CMake the system is Linux-like (Emscripten target)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_ZLIB=1 -s ALLOW_MEMORY_GROWTH=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_ZLIB=1 -s ALLOW_MEMORY_GROWTH=1")

# Force static linking for libraries
set(BUILD_SHARED_LIBS OFF)

# Emscripten OpenGL/WebGL settings
set(OpenGL_GL_PREFERENCE GLVND)
add_definitions(-D__EMSCRIPTEN__)

# Paths
if(NOT DEFINED EMSCRIPTEN_ROOT_PATH)
    message(FATAL_ERROR "EMSCRIPTEN_ROOT_PATH not defined! Make sure you source emsdk_env.sh / emsdk_env.bat")
endif()
include_directories(${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include)

# Preload assets helper macro
macro(add_emscripten_preload target assets_dir)
    set_source_files_properties(${assets_dir} PROPERTIES
        COMPILE_FLAGS "--preload-file ${assets_dir}@/assets")
endmacro()

if(WIN32)
    set(EMCMAKE_COMMAND emcmake.bat)
else()
    set(EMCMAKE_COMMAND emcmake)
endif()

message(STATUS "Configuring for Emscripten build (WebAssembly)")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Exec suffix: ${CMAKE_EXECUTABLE_SUFFIX}")
