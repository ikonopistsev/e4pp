cmake_minimum_required(VERSION 3.18)

project(e4pp VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(e4pp INTERFACE) 

target_include_directories(e4pp INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# i don't know for what
# add_library(e4pp::e4pp ALIAS e4pp)

option(E4PP_BUILD_EXAMPLES "build examples" OFF)
option(E4PP_STATIC_LIBEVENT "e4pp static libevent" OFF)
option(E4PP_WITH_PRIVATE "build private example" OFF)

if (E4PP_BUILD_EXAMPLES)
    add_subdirectory(ext/wslay)
    if (E4PP_STATIC_LIBEVENT)
        set(EVENT__LIBRARY_TYPE "STATIC" CACHE STRING "libevent static")
        set(EVENT__DISABLE_OPENSSL ON CACHE BOOL "libevent no openssl")
        set(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "libevent no debug")
        set(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "libevent no benchmark")
        set(EVENT__DISABLE_MBEDTLS ON CACHE BOOL "libevent no mbedtls")
        set(EVENT__DISABLE_TESTS ON CACHE BOOL "libevent no tests")
        set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "libevent no samples")
        set(EVENT__DISABLE_REGRESS ON CACHE BOOL "libevent no regress")
        set(EVENT__DOXYGEN OFF CACHE BOOL "libevent no doxygen")
        add_subdirectory(ext/libevent)
    endif()
    add_subdirectory(example)
endif()

# Installation
# ------------
include(GNUInstallDirs)

# Install the library
install(TARGETS e4pp 
    EXPORT e4ppTargets 
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install the headers
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CPack)
