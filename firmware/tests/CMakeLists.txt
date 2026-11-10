# Host-side unit tests of the hardware-independent logic.
#   cmake -S firmware/tests -B firmware/build/tests && cmake --build firmware/build/tests && ctest --test-dir firmware/build/tests
cmake_minimum_required(VERSION 3.21)
project(devhub_fw_tests C)

set(CMAKE_C_STANDARD 11)
enable_testing()

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src)
add_library(logic STATIC ${SRC}/app/ports.c ${SRC}/app/relay.c ${SRC}/proto/proto.c)
target_include_directories(logic PUBLIC ${SRC} ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(logic PUBLIC -Wall -Wextra -Wno-unused-parameter -g)

foreach(t test_ports test_relay test_proto)
  add_executable(${t} ${t}.c)
  target_link_libraries(${t} PRIVATE logic)
  add_test(NAME ${t} COMMAND ${t})
endforeach()
