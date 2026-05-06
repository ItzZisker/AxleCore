add_executable(HelloWindowGirl HelloWindowGirl.cpp)
target_link_libraries(HelloWindowGirl PUBLIC ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)