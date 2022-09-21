cmake_minimum_required(VERSION 3.18)

project(e4ppexample VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  set(LIBEVENT_STATIC_LINK TRUE)
  # on windows change hints
  find_package (libevent 2.2 REQUIRED HINTS "../../libevent/build-threadtux")
endif()

add_executable(evheap evheap.cpp)

if (WIN32)
  target_compile_definitions(evheap PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_include_directories(evheap PUBLIC ${LIBEVENT_INCLUDE_DIR})
  target_link_libraries(evheap PRIVATE ${LIBEVENT_LIBRARIES} e4pp)
else()
  target_link_libraries(evheap PRIVATE event event_pthreads e4pp)
endif()

add_executable(evhttpsrv evhttpsrv.cpp)

if (WIN32)
  target_compile_definitions(evhttpsrv PRIVATE -D_CRT_SECURE_NO_WARNINGS)
  target_include_directories(evhttpsrv PUBLIC ${LIBEVENT_INCLUDE_DIR})
  target_link_libraries(evhttpsrv PRIVATE ${LIBEVENT_LIBRARIES} e4pp)
else()
  target_link_libraries(evhttpsrv PRIVATE event e4pp)
endif()
