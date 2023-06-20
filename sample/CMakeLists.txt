add_executable(MySample sample.cpp)

target_link_libraries(MySample ldmonitor)

target_include_directories(MySample PRIVATE ${PROJECT_SOURCE_DIR}/include/)
