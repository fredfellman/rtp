cmake_minimum_required(VERSION 3.2)
project(rtp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wno-unused-variable  -lpthread -D_REENTRANT -std=gnu11 -lavahi-client -lavahi-common")


add_executable(rtp main.c src/zeroconf_browse.h src/zeroconf_browse.c)