# Depends on cmake/modules/**

# Automatically downloads and builds Assimp
# Works on Windows, Linux, macOS, Android, WebAssembly (Emscripten)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

include(ExternalProject)

set(OPENALSOFT_PREFIX ${CMAKE_SOURCE_DIR}/external/openalsoft)
set(OPENALSOFT_SRC_DIR ${OPENALSOFT_PREFIX}/src)
set(OPENALSOFT_BUILD_DIR ${OPENALSOFT_PREFIX}/${AX_PLATFORM_UNIFIED_NAME}/build)
set(OPENALSOFT_INSTALL_DIR ${OPENALSOFT_PREFIX}/${AX_PLATFORM_UNIFIED_NAME}/install)
set(OPENALSOFT_EXTRA_ARGS "")

# ExternalProject Add
ExternalProject_Add(
    openalsoft
    PREFIX ${OPENALSOFT_PREFIX}
    STAMP_DIR ${OPENALSOFT_PREFIX}/openalsoft-stamp
    GIT_REPOSITORY https://github.com/kcat/openal-soft
    GIT_TAG 1.24.3
    SOURCE_DIR ${OPENALSOFT_SRC_DIR}
    BINARY_DIR ${OPENALSOFT_BUILD_DIR}
    INSTALL_DIR ${OPENALSOFT_INSTALL_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${OPENALSOFT_INSTALL_DIR}
        -DALSOFT_INSTALL=ON
    UPDATE_DISCONNECTED 1
    BUILD_ALWAYS 1
    BUILD_IN_SOURCE 0
)

add_library(ALSoft SHARED IMPORTED GLOBAL)

# Create imported target for linking
if (WIN32)
    set_target_properties(ALSoft PROPERTIES
        IMPORTED_LOCATION "${OPENALSOFT_INSTALL_DIR}/bin/OpenAL32.dll"
        IMPORTED_IMPLIB  "${OPENALSOFT_INSTALL_DIR}/lib/libOpenAL32.dll.a"
    )
elseif (UNIX AND NOT APPLE)
    set_target_properties(ALSoft PROPERTIES
        IMPORTED_LOCATION "${OPENALSOFT_INSTALL_DIR}/lib/libopenal.so"
    )
endif()
