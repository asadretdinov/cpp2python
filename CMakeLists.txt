cmake_minimum_required(VERSION 3.12)

project(cpp2python_project)

# For C++17
set(CMAKE_CXX_STANDARD 17)

# static runtime linking
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")


add_subdirectory(src)