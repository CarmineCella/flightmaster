cmake_minimum_required(VERSION 2.8.9)
project (flightmaster)

SET(CMAKE_CXX_FLAGS "-Wall -g -O2 -std=c++11 -I . -Wno-return-type-c-linkage")

set (LIBS ${LIBS} curl) 

add_executable(flightmaster flightmaster.cpp flightmaster.h utilities.h)
add_executable(usfixes usfixes.cpp utilities.h)
target_link_libraries (flightmaster dl ${LIBS})
target_link_libraries (usfixes dl ${LIBS})
INSTALL(PROGRAMS ../db/airport-frequencies.csv ../db/airports.csv ../db/runways.csv ../db/navaids.csv DESTINATION $ENV{HOME}/.flightmaster)
INSTALL(
    TARGETS flightmaster usfixes
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION $ENV{HOME}/.flightmaster
    ARCHIVE DESTINATION $ENV{HOME}/.flightmaster)
