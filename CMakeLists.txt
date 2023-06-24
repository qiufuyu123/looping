cmake_minimum_required(VERSION 3.10)

# set the project name
project(looping)
add_compile_options(-O2 -w)
aux_source_directory(./src DIR_SRCS)
# add the executable
add_executable(looping ${DIR_SRCS})