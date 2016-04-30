#
# Rho's CMakeLists file.
#

cmake_minimum_required(VERSION 2.8)
project(Rho)

file(GLOB_RECURSE RHO_HEADERS ${CMAKE_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE RHO_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/)
set(LIB_SUFFIX "64")
include_directories(include/)
add_executable(rho ${RHO_SOURCES} ${RHO_HEADERS})

#
# Dependencies.
#
#-------------------------------------------------------------------------------

# Boost
find_package(Boost COMPONENTS system program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(rho ${Boost_LIBRARIES})

# GMP
find_package(GMP)
include_directories(${GMP_INCLUDE_DIR})
target_link_libraries(rho ${GMP_LIBRARIES})

# MPFR
find_package(MPFR)
include_directories(${MPFR_INCLUDES})
target_link_libraries(rho ${MPFR_LIBRARIES})

#-------------------------------------------------------------------------------

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g -Wall")


