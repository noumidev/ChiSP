# Require CMake 3.16
cmake_minimum_required(VERSION 3.16)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Werror -Wextra)

# Default to debug build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

project(ChiSP CXX)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/external/plog)

# Set source files
set(SOURCES
    src/main.cpp
    src/common/file.cpp
    src/sys/memory.cpp
    src/sys/emulator.cpp
)

# Set header files
set(HEADERS
    include/common/file.hpp
    include/common/types.hpp
    include/sys/memory.hpp
    include/sys/emulator.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE plog)
