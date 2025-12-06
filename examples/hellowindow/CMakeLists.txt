add_executable(HelloGLWindowX11 HelloGLWindowX11.cpp)
target_link_libraries(HelloGLWindowX11 PUBLIC ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)