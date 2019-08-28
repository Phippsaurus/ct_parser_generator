cmake_minimum_required(VERSION 3.10.2)

project(ct_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -pedantic \
-Werror -Wfatal-errors -Wno-error=ignored-qualifiers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

find_package(Threads REQUIRED)

add_executable(expression_parser
	src/expression_parser_main.cpp)

target_include_directories(expression_parser PRIVATE
	${PROJECT_SOURCE_DIR}/include/)

target_link_libraries(expression_parser
	${CMAKE_THREAD_LIBS_INIT})

add_executable(json_parser
	src/json_parser_main.cpp)

target_include_directories(json_parser PRIVATE
	${PROJECT_SOURCE_DIR}/include/)

target_link_libraries(json_parser
	${CMAKE_THREAD_LIBS_INIT})
