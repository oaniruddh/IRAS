cmake_minimum_required(VERSION 3.16)
project(IRAS_SystemMonitor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Source files
set(SOURCES
    main.cpp
    SystemMonitor.cpp
    Logger.cpp
)

# Header files
set(HEADERS
    SystemMonitor.h
    Config.h
    Logger.h
)

# Create the executable
add_executable(SystemMonitor ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(SystemMonitor 
    comctl32
    pdh
    psapi
    iphlpapi
    advapi32
    ws2_32
)

# Compiler-specific options
if(MSVC)
    target_compile_options(SystemMonitor PRIVATE /W4)
    set_target_properties(SystemMonitor PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
else()
    target_compile_options(SystemMonitor PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy resources to output directory (if any)
# file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# Installation
install(TARGETS SystemMonitor
    RUNTIME DESTINATION bin
)

# CPack configuration for installer
set(CPACK_PACKAGE_NAME "IRAS System Monitor")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "IRAS")
set(CPACK_PACKAGE_DESCRIPTION "Advanced System Monitoring Tool")
set(CPACK_PACKAGE_EXECUTABLES "SystemMonitor;System Monitor")

include(CPack)
