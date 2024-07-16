cmake_minimum_required(VERSION 3.18)

project(e4ppexample VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(evheap evheap.cpp)
add_executable(evhttpsrv evhttpsrv.cpp)

#target_include_directories(evwsclient PRIVATE "../ext/libevent/include")

if (WIN32)
  target_compile_definitions(evheap PRIVATE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
  target_compile_definitions(evhttpsrv PRIVATE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
  target_compile_definitions(evwsclient PRIVATE -D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if (E4PP_STATIC_LIBEVENT)
  target_link_libraries(evheap PRIVATE e4pp event_core_static event_extra_static event_pthreads_static)
  target_link_libraries(evhttpsrv PRIVATE e4pp event_core_static event_extra_static event_pthreads_static)
  target_link_libraries(evwsclient PRIVATE e4pp event_core_static event_extra_static event_pthreads_static wslay ssl crypto)
else()
  target_link_libraries(evheap PRIVATE e4pp event_core event_extra event_pthreads)
  target_link_libraries(evhttpsrv PRIVATE e4pp event_core event_extra event_pthreads)
  target_link_libraries(evwsclient PRIVATE e4pp event_core event_extra event_pthreads wslay ssl crypto)
endif()
