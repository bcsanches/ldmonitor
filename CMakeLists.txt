cmake_minimum_required (VERSION 3.14)

#main project
project (ldmonitor)

set(CMAKE_CXX_STANDARD 17)

set(VERSION_MAJOR "0" )
set(VERSION_MINOR "1" )
set(VERSION_PATCH "0" )
set(LDMONITOR_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

add_compile_definitions(LDMONITOR_VERSION=\"${LDMONITOR_VERSION}\")

option(LDMONITOR_SAMPLE_BUILD "Build Sample" OFF)
if (LDMONITOR_SAMPLE_BUILD)
	add_subdirectory(sample)    
endif()
add_subdirectory(sample)    

option(LD_MONITOR_PACKAGE_TESTS "Build the tests" OFF)
if(LD_MONITOR_PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(utests)
endif()

set( CMAKE_EXPORT_COMPILE_COMMANDS on)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(src)

set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


include(InstallRequiredSystemLibraries)

install(TARGETS ldmonitor)
