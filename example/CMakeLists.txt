cmake_minimum_required(VERSION 3.10)

project(e4ppexample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(evheap evheap.cpp)

if (WIN32)
  target_link_libraries(evheap event e4pp)
else()
  target_link_libraries(evheap event event_pthreads pthread e4pp)
endif()