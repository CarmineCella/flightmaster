cmake_minimum_required(VERSION 2.8.9)
project (fplan)

SET(CMAKE_CXX_FLAGS "-Wall -g -O2 -std=c++11 -I . -Wno-return-type-c-linkage")

set (LIBS ${LIBS} curl) 

add_executable(fplan fplan.cpp fplan.h)
target_link_libraries (fplan dl ${LIBS})
INSTALL(PROGRAMS ../db/airport-frequencies.csv ../db/airports.csv ../db/runways.csv ../db/FIX.txt DESTINATION $ENV{HOME}/.fplan)
INSTALL(
    TARGETS fplan
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION $ENV{HOME}/.fplan
    ARCHIVE DESTINATION $ENV{HOME}/.fplan)
