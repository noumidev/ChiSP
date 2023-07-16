cmake_minimum_required(VERSION 3.8)

project(ChiSP CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile options
add_compile_options(-Wall -Wextra)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(cryptopp-cmake)

# Set source and header files
set(SOURCES
    src/main.cpp
    src/core/ddr.cpp
    src/core/gpio.cpp
    src/core/memory.cpp
    src/core/nand.cpp
    src/core/psp.cpp
    src/core/syscon.cpp
    src/core/allegrex/allegrex.cpp
    src/core/allegrex/cop0.cpp
    src/core/allegrex/interpreter.cpp
    src/core/crypto/kirk.cpp
)

set(HEADERS
    src/common/file.hpp
    src/common/types.hpp
    src/core/ddr.hpp
    src/core/gpio.hpp
    src/core/memory.hpp
    src/core/nand.hpp
    src/core/psp.hpp
    src/core/syscon.hpp
    src/core/allegrex/allegrex.hpp
    src/core/allegrex/cop0.hpp
    src/core/allegrex/interpreter.hpp
    src/core/crypto/kirk.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp)
