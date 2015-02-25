# 
# Rho's CMakeLists file.
# 

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(Rho)

FILE(GLOB_RECURSE Rho_Headers ${CMAKE_SOURCE_DIR}/include/*.hpp)
FILE(GLOB_RECURSE Rho_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

ADD_SUBDIRECTORY(cmake)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/)
SET(LIB_SUFFIX "64")
INCLUDE_DIRECTORIES(include/)
ADD_EXECUTABLE(rho ${Rho_SOURCES} ${Rho_Headers})

# 
# Dependencies.
#
#-------------------------------------------------------------------------------

# GMP
FIND_PACKAGE(GMP)
INCLUDE_DIRECTORIES(${GMP_INCLUDE_DIR})

# MPFR
FIND_PACKAGE(MPFR)
INCLUDE_DIRECTORIES(${MPFR_INCLUDES})

# Readline
FIND_PACKAGE(Readline)
INCLUDE_DIRECTORIES(${Readline_INCLUDE_DIR})

#-------------------------------------------------------------------------------

TARGET_LINK_LIBRARIES(rho
  ${GMP_LIBRARIES} ${MPFR_LIBRARIES} ${Readline_LIBRARY})

SET(CMAKE_CXX_FLAGS "-Wall -O0 -g -std=c++11")

