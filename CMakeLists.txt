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
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GIO REQUIRED gio-unix-2.0)
pkg_search_module(SYSTEMD REQUIRED libsystemd)
pkg_search_module(SSL REQUIRED openssl)

# Use C++11, but without GNU or other extensions
set(CMAKE_CXX_STANDARD 14)

add_executable(fioras src/main.cpp src/checks.cpp src/check_aklite.cpp src/check_systemd.cpp src/check_compose.cpp src/httpd.cpp src/sysinfo.cpp)
target_include_directories(fioras PRIVATE ${CMAKE_SOURCE_DIR}/third_party ${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS} ${SYSTEMD_INCLUDE_DIRS})
target_link_libraries(fioras -lpthread ${CURL_LIBRARIES} ${Boost_LIBRARIES} ${GLIB_LDFLAGS} ${GIO_LDFLAGS} ${SYSTEMD_LDFLAGS} ${SSL_LDFLAGS})
install(TARGETS fioras RUNTIME DESTINATION bin)

add_custom_target(clang-check-format
	${CMAKE_SOURCE_DIR}/tools/run-clang-format --clang-format-executable=clang-format-10 ${CMAKE_SOURCE_DIR}/src/*
)
add_custom_target(clang-format
	${CMAKE_SOURCE_DIR}/tools/run-clang-format -i --clang-format-executable=clang-format-10 ${CMAKE_SOURCE_DIR}/src/*
)
add_custom_target(clang-tidy
	${CMAKE_SOURCE_DIR}/tools/run-clang-tidy ${CMAKE_SOURCE_DIR}/src
)
