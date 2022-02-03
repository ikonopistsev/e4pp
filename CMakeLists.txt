cmake_minimum_required(VERSION 3.10)

project(e4pp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(e4pp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include) 

target_include_directories(
    e4pp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# i don't know for what
add_library(e4pp::e4pp ALIAS e4pp)

add_subdirectory(example)

# Installation
# ------------
include(GNUInstallDirs)

# Install the library
install(
    TARGETS e4pp
    EXPORT e4ppTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install the headers
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CPack)
