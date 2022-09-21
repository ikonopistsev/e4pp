cmake_minimum_required(VERSION 3.18)

project(e4ppexample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  set(LIBEVENT_STATIC_LINK TRUE)
endif()

# on windows change hints
find_package (libevent 2.2 REQUIRED HINTS "../../libevent/build-threadtux")

add_executable(evheap evheap.cpp)

if (WIN32)
  target_compile_definitions(evheap PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_include_directories(evheap PUBLIC ${LIBEVENT_INCLUDE_DIR})
  target_link_libraries(evheap ${LIBEVENT_LIBRARIES} e4pp)
else()
  target_link_libraries(evheap ${LIBEVENT_LIBRARIES} pthread e4pp)
endif()

add_executable(evhttpsrv evhttpsrv.cpp)

if (WIN32)
  target_compile_definitions(evhttpsrv PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_include_directories(evhttpsrv PUBLIC ${LIBEVENT_INCLUDE_DIR})
  target_link_libraries(evhttpsrv ${LIBEVENT_LIBRARIES} e4pp)
else()
  target_link_libraries(evhttpsrv ${LIBEVENT_LIBRARIES} pthread e4pp)
endif()
