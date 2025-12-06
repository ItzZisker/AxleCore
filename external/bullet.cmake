# Depends on cmake/modules/**

# Automatically downloads and builds Bullet Physics
# with custom options for AxiomCore.
# Works on Windows, Linux, macOS, Android, Emscripten(WASM)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

include(ExternalProject)

# User-configurable install dirs
set(BULLET_PREFIX ${CMAKE_SOURCE_DIR}/external/bullet)
set(BULLET_SRC_DIR ${BULLET_PREFIX}/src)
set(BULLET_BUILD_DIR ${BULLET_PREFIX}/${AX_PLATFORM_UNIFIED_NAME}/build)
set(BULLET_INSTALL_DIR ${BULLET_PREFIX}/${AX_PLATFORM_UNIFIED_NAME}/install)

# Extra CMake args
set(BULLET_EXTRA_ARGS "")

# Unix-like <-> Linux
if (UNIX AND NOT APPLE AND NOT ANDROID) 
    # Position Independent Code for Linux static builds
    list(APPEND BULLET_EXTRA_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
endif()

# For Android toolchain, Emscripten, or cross-compilation
if (ANDROID OR WEB)
    list(APPEND BULLET_EXTRA_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif()

# ExternalProject Add
ExternalProject_Add(
    bullet
    PREFIX ${BULLET_PREFIX}
    STAMP_DIR ${BULLET_PREFIX}/bullet-stamp
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG master
    SOURCE_DIR ${BULLET_SRC_DIR}
    BINARY_DIR ${BULLET_BUILD_DIR}
    INSTALL_DIR ${BULLET_INSTALL_DIR}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${BULLET_INSTALL_DIR}
        -DBUILD_BULLET2_DEMOS=OFF
        -DBUILD_BULLET3=ON
        -DBUILD_CLSOCKET=OFF
        -DBUILD_CPU_DEMOS=OFF
        -DBUILD_ENET=OFF
        -DBUILD_EXTRAS=OFF
        -DBUILD_OPENGL3_DEMOS=OFF
        -DBUILD_PYBULLET=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_UNIT_TESTS=OFF
        -DBULLET2_MULTITHREADING=OFF
        -DENABLE_VHACD=ON
        -DINSTALL_CMAKE_FILES=ON
        -DINSTALL_LIBS=ON
        -DLIB_DESTINATION=lib
        -DINTERNAL_CREATE_DISTRIBUTABLE_MSVC_PROJECTFILES=OFF
        -DINTERNAL_UPDATE_SERIALIZATION_STRUCTURES=OFF
        -DUSE_DOUBLE_PRECISION=OFF
        -DUSE_GLUT=OFF
        -DUSE_GRAPHICAL_BENCHMARK=OFF
        -DUSE_MSVC_INCREMENTAL_LINKING=OFF
        -DUSE_MSVC_RELEASE_RUNTIME_ALWAYS=OFF
        -DUSE_MSVC_RUNTIME_LIBRARY_DLL=OFF
        -DUSE_OPENVR=OFF
        -DUSE_SOFT_BODY_MULTI_BODY_DYNAMICS_WORLD=ON
        ${BULLET_EXTRA_ARGS}
    UPDATE_DISCONNECTED 1
    BUILD_ALWAYS 1
    BUILD_IN_SOURCE 0
)

set(BULLET_LIBS
    "${BULLET_INSTALL_DIR}/lib/libBullet2FileLoader.a"
    "${BULLET_INSTALL_DIR}/lib/libBullet3OpenCL_clew.a"
    "${BULLET_INSTALL_DIR}/lib/libBullet3Common.a"
    "${BULLET_INSTALL_DIR}/lib/libBullet3Geometry.a"
    "${BULLET_INSTALL_DIR}/lib/libBulletCollision.a"
    "${BULLET_INSTALL_DIR}/lib/libBullet3Collision.a"
    "${BULLET_INSTALL_DIR}/lib/libBulletInverseDynamics.a"
    "${BULLET_INSTALL_DIR}/lib/libBulletDynamics.a"
    "${BULLET_INSTALL_DIR}/lib/libBullet3Dynamics.a"
    "${BULLET_INSTALL_DIR}/lib/libBulletSoftBody.a"
    "${BULLET_INSTALL_DIR}/lib/libLinearMath.a"
)
add_library(Bullet STATIC IMPORTED GLOBAL)
set_target_properties(Bullet PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    IMPORTED_LOCATION "${BULLET_INSTALL_DIR}/lib/libBulletDynamics.a"
    INTERFACE_LINK_LIBRARIES "${BULLET_LIBS}"
)