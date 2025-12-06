include(FetchContent)

# SDL3
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/sdl3-build)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG release-3.2.26
    )
    FetchContent_MakeAvailable(SDL3)
endif()

# FreeType
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/freetype-build)
    set(CMAKE_POLICY_VERSION_MINIMUM_PREV ${CMAKE_POLICY_VERSION_MINIMUM})
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

    FetchContent_Declare(
        freetype
        URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz
    )
    FetchContent_MakeAvailable(freetype)

    set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_POLICY_VERSION_MINIMUM_PREV})
endif()

# === GLM ===
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/glm-build)
    message(STATUS "Downloading GLM...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.2
    )
    FetchContent_MakeAvailable(glm)
endif()
include_directories(${glm_SOURCE_DIR})

# === ImGui ===
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/imgui-build)
    message(STATUS "Downloading ImGui...")
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.5
    )
    FetchContent_MakeAvailable(imgui)
endif()
set(AX_IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)

# === STB ===
if(NOT EXISTS ${CMAKE_BINARY_DIR}/_deps/stb-build)
    message(STATUS "Downloading STB...")
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(stb)
endif()
include_directories(${stb_SOURCE_DIR})