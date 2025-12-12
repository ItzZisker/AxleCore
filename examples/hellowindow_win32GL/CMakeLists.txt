add_executable(HelloGLWindowWin32 HelloGLWindowWin32.cpp)
target_link_libraries(HelloGLWindowWin32 PUBLIC ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)