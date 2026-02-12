cmake_minimum_required(VERSION 3.20)

project(AxleCore VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Include directories ===
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

if (AX_BUILD_EMWEB)
    include_directories(${EMSCRIPTEN_ROOT_PATH}/cache/sysroot/include)
endif()

# Default options, compiler flags, Link ordinary libs
include(cmake/Common.cmake)
include(cmake/Dependencies.cmake)

# These scripts download/build/install heavy dependencies into external/
include(external/bullet.cmake)
include(external/assimp.cmake)

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

    src/graphics/cmd/GL/AX_GLCommandList.cpp
    src/graphics/cmd/GL/AX_GLGraphicsBackend.cpp
    
	src/graphics/base/material/AX_Material.cpp
    src/graphics/base/material/AX_MaterialMetadata.cpp
	
    src/graphics/base/texture/AX_Texture.cpp
    src/graphics/base/texture/AX_TextureSTB.cpp
    src/graphics/base/texture/AX_TextureBRDFLUT.cpp
    src/graphics/base/texture/AX_TextureDXT.cpp
    src/graphics/base/texture/AX_TextureASTC.cpp

    external/glad/glad.c
)

if (AX_BUILD_EMWEB)
    add_library(${PROJECT_NAME} STATIC ${PROJECT_SRC})
else()
    add_library(${PROJECT_NAME} SHARED ${PROJECT_SRC})
endif()

if (AX_IMPL_LAYER_X11)
    find_package(X11 REQUIRED) # Base
    find_library(XRANDR_LIB Xrandr) # Components
    find_library(XI_LIB Xi)
    find_library(DL_LIB dl)

    if (X11_FOUND AND XRANDR_LIB AND XI_LIB AND DL_LIB)
        message(STATUS "X11 Application Layer Support")

        add_compile_definitions(__AX_PLATFORM_X11__)
        target_include_directories(${PROJECT_NAME} PUBLIC ${X11_INCLUDE_DIR})
        target_link_libraries(${PROJECT_NAME} PUBLIC X11::X11 ${XRANDR_LIB} ${XI_LIB} ${DL_LIB})
    endif()
endif()

if (WIN32 AND AX_IMPL_LAYER_WIN32)
    message(STATUS "Win32 Application Layer Support")

    add_compile_definitions(__AX_PLATFORM_WIN32__)
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

# Apply common libraries
set(AX_COMMON_LIBS "")

if (AX_IMPL_GRAPHICS_GL)
    include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)

    find_package(OpenGL REQUIRED)

    if (UNIX)  # BSD, Linux, etc.
        list(APPEND AX_COMMON_LIBS OpenGL::GL)
    elseif (WIN32)  # Windows
        list(APPEND AX_COMMON_LIBS opengl32)
    elseif (APPLE)
        # Todo: Write Objective-C .mm file and implement the IApplication interface header on Macbook.
        list(APPEND AX_COMMON_LIBS "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
    endif()
    add_compile_definitions(__AX_GRAPHICS_GL__)
    target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
endif()

if (AX_IMPL_GRAPHICS_DX11)
    if (WIN32)
        list(APPEND AX_COMMON_LIBS d3d11 dxgi dxguid)
        add_compile_definitions(__AX_GRAPHICS_DX11__)
    else()
        message(FATAL_ERROR "DirectX 11 is only supported on Windows.")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${AX_COMMON_LIBS})

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

# Precompiled headers for quicker compile time
# target_precompile_headers(${PROJECT_NAME} PRIVATE
#     "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/src/AX_CorePCH.hpp>"
# )

if (AX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "AxleCore version: ${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform: ${AX_PLATFORM}")

if(AX_BUILD_EMWEB)
    target_link_options(sample_executable PRIVATE
        "--preload-file assets@/assets"
        "-s MIN_WEBGL_VERSION=2"
        "-s MAX_WEBGL_VERSION=2"
        "-s USE_WEBGL2=1"
        "-s FULL_ES3=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s ASYNCIFY"
        "-s EXIT_RUNTIME=0"
        "-s USE_ZLIB=1"
        "-s USE_SDL3=1" # TODO: Remove this later -> Once we had emscripten & Android NDK implementation
        "-sDISABLE_EXCEPTION_CATCHING=0"
    )
    message(STATUS "Building for WebAssembly (Emscripten)")
endif()
