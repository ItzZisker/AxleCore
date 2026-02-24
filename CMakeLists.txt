cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(AxleCore VERSION 0.1.0 LANGUAGES C CXX)

# Default options, compiler flags, Link ordinary libs
include(cmake/Common.cmake)
include(cmake/Dependencies.cmake)

# These scripts download/build/install heavy dependencies into external/
if (AX_IMPL_BULLET3)
    include(external/bullet.cmake)
endif()

if (AX_IMPL_ASSIMP)
    include(external/assimp.cmake)
endif()

if (AX_IMPL_AUDIO_SOFTOPENAL)
    include(external/openalsoft.cmake)
endif()

# === Audio Source Files ===
set(AUDIO_SRC
	src/audio/AL/AX_ALAudioContext.cpp
	src/audio/AL/AX_ALAudioListener.cpp
	src/audio/AL/AX_ALAudioSource.cpp
    
	src/audio/AL/buffer/AX_ALAudioBuffer.cpp
	src/audio/AL/buffer/AX_ALAudioBufferSource.cpp
	src/audio/AL/buffer/AX_ALAudioBufferPlayer.cpp

	src/audio/AL/stream/AX_ALAudioStreamVorbis.cpp
	src/audio/AL/stream/AX_ALAudioStreamVorbisSource.cpp
	src/audio/AL/stream/AX_ALAudioStreamVorbisPlayer.cpp

	src/audio/data/AX_AudioWAV.cpp
	src/audio/data/AX_AudioOGG.cpp

	src/audio/AX_AudioUtils.cpp
)

# === Source files ===
set(PROJECT_SRC
    src/external/AX_stb.c

    src/core/app/AX_Application.cpp

    src/core/concurrency/AX_ThreadCycler.cpp
    src/core/concurrency/AX_TaskQueue.cpp
    # src/core/concurrency/AX_Future.cpp
    # src/core/concurrency/AX_MainThread.cpp
    # src/core/concurrency/AX_TaskQueueRunnable.cpp

    src/core/ctx/DX11/AX_RenderContextDX11Win32.cpp

    src/core/ctx/GL/AX_RenderContextGLWin32.cpp
    src/core/ctx/GL/AX_RenderContextGLX11.cpp

    src/core/window/AX_IWindow.cpp
    src/core/window/AX_WindowWin32.cpp
    src/core/window/AX_WindowX11.cpp

    src/data/AX_DataSerializer.cpp
    src/data/AX_DataDeserializer.cpp
    src/data/AX_DataStreamImplBuffer.cpp
    src/data/AX_DataStreamImplFile.cpp
    src/data/AX_DataTemplates.cpp

    # src/assets/AX_AssetImporter.cpp
    # src/assets/AX_AssetExporter.cpp
    # src/assets/AX_AssetManager.cpp
    # src/assets/AX_AssetPacker.cpp
	
    ${AUDIO_SRC}

    src/graphics/cmd/AX_Graphics.cpp

    src/graphics/cmd/GL/AX_GLCommandList.cpp
    src/graphics/cmd/GL/AX_GLGraphicsBackend.cpp
    
	src/graphics/base/material/AX_Material.cpp
    src/graphics/base/material/AX_MaterialMetadata.cpp
	
    src/graphics/base/texture/AX_Texture.cpp
    src/graphics/base/texture/AX_TextureSTB.cpp
    src/graphics/base/texture/AX_TextureBRDFLUT.cpp
    src/graphics/base/texture/AX_TextureDXT.cpp
    src/graphics/base/texture/AX_TextureASTC.cpp

    # external/glad/gles2.c
)

if (UNIX AND AX_IMPL_LAYER_X11)
    list(APPEND PROJECT_SRC external/glad/glx.c)
    message(STATUS "X11 Application Layer Support")
    add_compile_definitions(__AX_PLATFORM_X11__)
endif()

if (WIN32 AND AX_IMPL_LAYER_WIN32)
    message(STATUS "Win32 Application Layer Support")
    add_compile_definitions(__AX_PLATFORM_WIN32__)
endif()

if (AX_IMPL_GRAPHICS_GL)
    list(APPEND PROJECT_SRC external/glad/gl.c)
    message(STATUS "OpenGL Graphics Backend Support")
    add_compile_definitions(__AX_GRAPHICS_GL__)
endif()

if (WIN32 AND AX_IMPL_GRAPHICS_DX11)
    add_compile_definitions(__AX_GRAPHICS_DX11__)
endif()

# Precompiled headers for quicker compile time
# target_precompile_headers(${PROJECT_NAME} PRIVATE
#     "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/src/AX_CorePCH.hpp>"
# )

if (AX_BUILD_EMWEB)
    add_library(${PROJECT_NAME} STATIC ${PROJECT_SRC})
else()
    add_library(${PROJECT_NAME} STATIC ${PROJECT_SRC})
endif()

if (UNIX AND AX_IMPL_LAYER_X11)
    find_package(X11 REQUIRED) # Base
    find_library(XRANDR_LIB Xrandr REQUIRED) # Components
    find_library(XI_LIB Xi REQUIRED)
    find_library(DL_LIB dl REQUIRED)

    target_include_directories(${PROJECT_NAME} PUBLIC ${X11_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PUBLIC X11::X11 ${XRANDR_LIB} ${XI_LIB} ${DL_LIB})
endif()

if (WIN32 AND AX_IMPL_LAYER_WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        user32
        gdi32
        winmm
        imm32
        shell32
        ole32
        oleaut32
        uuid
        version
        setupapi
    )
endif()

if (AX_IMPL_GRAPHICS_GL)
    include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/external/glad/include)

    find_package(OpenGL REQUIRED)

    if (UNIX)  # BSD, Linux, etc.
        target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL)
    elseif (WIN32)  # Windows
        target_link_libraries(${PROJECT_NAME} PUBLIC opengl32)
    elseif (APPLE)
        # Todo: Write Objective-C .mm file and implement the IApplication interface header on Macbook.
        target_link_libraries(${PROJECT_NAME} PUBLIC "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    endif()
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
endif()

if (WIN32 AND AX_IMPL_GRAPHICS_DX11)
    target_link_libraries(${PROJECT_NAME} PUBLIC d3d11 dxgi dxguid)
endif()

if (AX_IMPL_AUDIO_SOFTOPENAL)
    message("-- Audio OpenAL-soft support enabled.")

    target_link_libraries(${PROJECT_NAME} PUBLIC ALSoft)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENALSOFT_INSTALL_DIR}/include)
    add_compile_definitions(__AX_AUDIO_ALSOFT__)
endif()

if (AX_IMPL_ASSIMP)
    message("-- Assimp support enabled.")
    set(AX_ASSIMP_LINK Assimp)
    if (UNIX)
        set(AX_ASSIMP_LINK -Wl,--whole-archive Assimp -Wl,--no-whole-archive)
    endif()
    target_link_libraries(${PROJECT_NAME} PUBLIC ${AX_ASSIMP_LINK} z)
    add_compile_definitions(__AX_ASSETS_ASSIMP__)
endif()

if (AX_IMPL_BULLET3)
    message("-- Bullet3 support enabled.")
    set(AX_BULLET_LINK Bullet)
    if (UNIX)
        set(AX_BULLET_LINK -Wl,--whole-archive Bullet -Wl,--no-whole-archive)
    endif()
    target_link_libraries(${PROJECT_NAME} PUBLIC ${AX_BULLET_LINK})
    add_compile_definitions(_AX_PHYSICS_BULLET3__)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC slang)

# === Include directories ===
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(${PROJECT_NAME} PUBLIC ${glm_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${imgui_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${imgui_SOURCE_DIR}/backends)
target_include_directories(${PROJECT_NAME} PUBLIC ${stb_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${slang_SOURCE_DIR}/include)

if (AX_BUILD_EMWEB)
    include_directories(${PROJECT_NAME} PUBLIC ${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include)
endif()

if (AX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "AxleCore version: ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${AX_PLATFORM}")

# if(AX_BUILD_EMWEB)
#     target_link_options(sample_executable PRIVATE
#         "--preload-file assets@/assets"
#         "-s MIN_WEBGL_VERSION=2"
#         "-s MAX_WEBGL_VERSION=2"
#         "-s USE_WEBGL2=1"
#         "-s FULL_ES3=1"
#         "-s ALLOW_MEMORY_GROWTH=1"
#         "-s ASYNCIFY"
#         "-s EXIT_RUNTIME=0"
#         "-s USE_ZLIB=1"
#         "-s USE_SDL3=1" # TODO: Remove this later -> Once we had emscripten & Android NDK implementation
#         "-sDISABLE_EXCEPTION_CATCHING=0"
#     )
#     message(STATUS "Building for WebAssembly (Emscripten)")
# endif()
