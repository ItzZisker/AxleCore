add_executable(HelloDX11WindowWin32 HelloDX11WindowWin32.cpp)
target_link_libraries(HelloDX11WindowWin32 PUBLIC ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)