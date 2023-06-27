add_executable(MySample sample.cpp)

if(WIN32)
	target_link_libraries(MySample ldmonitor)
else(WIN32)
	target_link_libraries(MySample ldmonitor stdc++fs)
endif(WIN32)



target_include_directories(MySample PRIVATE ${PROJECT_SOURCE_DIR}/include/)
