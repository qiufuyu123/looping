cmake_minimum_required(VERSION 3.10)

# set the project name
project(looping)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
aux_source_directory(./src DIR_SRCS)
# add the executable
add_executable(looping ${DIR_SRCS})