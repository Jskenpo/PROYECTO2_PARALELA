cmake_minimum_required(VERSION 3.28)
project(PROYECTO2_PARALELA)

set(CMAKE_CXX_STANDARD 17)

# Add MPI Package to Project
find_package(MPI REQUIRED)

add_executable(PROYECTO2_PARALELA main.cpp)

# Add libraries for code completion and compiling
target_link_libraries(PROYECTO2_PARALELA PUBLIC MPI::MPI_CXX)
