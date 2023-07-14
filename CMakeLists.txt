cmake_minimum_required(VERSION 3.8)

project(ChiSP CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile options
add_compile_options(-Wall -Werror -Wextra)
set(CMAKE_BUILD_TYPE Debug)

# Set source and header files
set(SOURCES
    src/main.cpp
    src/core/memory.cpp
    src/core/psp.cpp
)

set(HEADERS
    src/common/file.hpp
    src/common/types.hpp
    src/core/memory.hpp
    src/core/psp.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
