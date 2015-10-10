cmake_minimum_required(VERSION 3.2)
project(rtp)
enable_testing()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wno-unused-variable  -lpthread -D_REENTRANT -std=gnu11 -lavahi-client -lavahi-common -lnanomsg")

add_subdirectory(test)


add_executable(rtp main.c src/zeroconf_browse.h src/zeroconf_browse.c src/zeroconf_queue.h src/debug.h src/zeroconf_queue.c src/zeroconf_common.h src/zeroconf_common.c src/zeroconf_publish.h src/zeroconf_publish.c src/zeroconf_rtp.h src/zeroconf_rtp.c)