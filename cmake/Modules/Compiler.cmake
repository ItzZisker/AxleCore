# Ensure C++20
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

# Compiler-specific flags
if(MSVC)
    add_compile_options(
		/W4
		/permissive-
		/MP
		/O2
	)
else()
    add_compile_options(
        -Wextra
        -Wpedantic
        -Wnon-virtual-dtor
		-O3
        -fPIC
    )
endif()

# Precompiled headers helper
function(add_precompiled_header target header)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND TARGET ${target})
        target_precompile_headers(${target} PRIVATE ${header})
    endif()
endfunction()

# SIMD optimizations
function(enable_simd target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -march=native)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /arch:AVX2)
    endif()
endfunction()
