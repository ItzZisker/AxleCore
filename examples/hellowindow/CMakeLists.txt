add_executable(HelloWindow HelloWindow.cpp)
target_link_libraries(HelloWindow PUBLIC ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)