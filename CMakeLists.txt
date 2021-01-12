# Copyright (c) 2021 Foundries.io
#
# SPDX-License-Identifier: MIT

cmake_minimum_required (VERSION 3.5)

project(fioras)

ADD_DEFINITIONS(-DGIT_COMMIT="${GIT_COMMIT}")
message(STATUS "Setting GIT_COMMIT to ${GIT_COMMIT}")

add_definitions('-g')
add_definitions('-Wall')

# find all required libraries
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(PkgConfig REQUIRED)

# Use C++11, but without GNU or other extensions
set(CMAKE_CXX_STANDARD 11)

add_executable(fioras src/main.cpp)
target_include_directories(fioras PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(fioras ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${GLIB_LDFLAGS})
install(TARGETS fioras RUNTIME DESTINATION bin)
