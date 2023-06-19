
add_library(ldmonitor DirectoryMonitor_win.cpp)

if(WIN32)

	target_compile_definitions(Common PRIVATE _CRT_SECURE_NO_WARNINGS)
    
    target_sources(Common PRIVATE
        ConsoleUtils_win.cpp
	DirectoryMonitor_win.cpp
        GuidUtils_win.cpp
        SerialPort_win.cpp
        SerialPort_win.h
		Sha1_win.cpp
    )

else(WIN32)

    target_sources(Common PRIVATE
		ConsoleUtils_linux.cpp
		GuidUtils_linux.cpp
		Sha1_linux.cpp
		SerialPort_linux.cpp
        SerialPort_linux.h
	)   
     
endif(WIN32)
		
target_compile_features(Common PUBLIC cxx_std_17)     

target_include_directories(Common
  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/fmt/include
  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/JsonCreator/include
  PUBLIC  ${DCCLite_SOURCE_DIR}/contrib/rapidjson/include
          ${DCCLite_SOURCE_DIR}/contrib/spdlog/include
  PUBLIC  ${DCCLite_SOURCE_DIR}/src/Embedded/lib/Shared
  PRIVATE ${DCCLite_SOURCE_DIR}/contrib/magic_enum/include	
)
