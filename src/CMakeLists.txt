if(WIN32)

  add_library(ldmonitor DirectoryMonitor.cpp DirectoryMonitor_win.cpp ${PROJECT_SOURCE_DIR}/include/ldmonitor/DirectoryMonitor.h)

else(WIN32)

  add_library(ldmonitor DirectoryMonitor.cpp DirectoryMonitor_linux.cpp ${PROJECT_SOURCE_DIR}/include/ldmonitor/DirectoryMonitor.h)
     
endif(WIN32)


target_include_directories(ldmonitor PRIVATE ${PROJECT_SOURCE_DIR}/include/ldmonitor)
		
#target_compile_features(Common PUBLIC cxx_std_17)     

#target_include_directories(Common
#  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/fmt/include
#  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/JsonCreator/include
#  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/rapidjson/include
#          ${DCCLite_SOURCE_DIR}/contrib/spdlog/include
#  PUBLIC  ${DCCLite_SOURCE_DIR}/src/Embedded/lib/Shared
#  PRIVATE ${DCCLite_SOURCE_DIR}/contrib/magic_enum/include	
#)
