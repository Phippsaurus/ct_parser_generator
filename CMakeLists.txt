cmake_minimum_required(VERSION 3.10.2)

project(ct_bison CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic \
-Werror -Wfatal-errors -Wno-error=ignored-qualifiers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

find_package(Threads REQUIRED)

add_executable(ct_bison
	src/main.cpp)

target_include_directories(ct_bison PRIVATE
	${PROJECT_SOURCE_DIR}/include/)

target_link_libraries(ct_bison
	${CMAKE_THREAD_LIBS_INIT})
