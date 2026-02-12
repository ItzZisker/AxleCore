include(FetchContent)

# FreeType
set(CMAKE_POLICY_VERSION_MINIMUM_PREV ${CMAKE_POLICY_VERSION_MINIMUM})
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    freetype
    URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.gz
)
FetchContent_MakeAvailable(freetype)

set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_POLICY_VERSION_MINIMUM_PREV})
unset(CMAKE_POLICY_VERSION_MINIMUM_PREV)

# === GLM ===
message(STATUS "Downloading GLM 1.0.2 ...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)
include_directories(${glm_SOURCE_DIR})

# === ImGui ===
message(STATUS "Downloading ImGui v1.92.5 ...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)
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
message(STATUS "Downloading STB...")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)
include_directories(${stb_SOURCE_DIR})

# === Slang ===
message(STATUS "Downloading Slang v2026.1.2 ...")

set(SLANG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_PYTHON OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_OPTIX OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_NVIDIA_EXTENSIONS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    slang
    GIT_REPOSITORY https://github.com/shader-slang/slang
    GIT_TAG v2026.1.2
)
FetchContent_MakeAvailable(slang)
include_directories(${slang_SOURCE_DIR}/include)