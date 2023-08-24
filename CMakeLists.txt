cmake_minimum_required(VERSION 3.8)

project(ChiSP CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compile options
add_compile_options(-O2 -Wall -Wextra)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(cryptopp-cmake)

# Set source and header files
set(SOURCES
    src/main.cpp
    src/core/ata.cpp
    src/core/ddr.cpp
    src/core/display.cpp
    src/core/dmacplus.cpp
    src/core/ge.cpp
    src/core/gpio.cpp
    src/core/hpremote.cpp
    src/core/intc.cpp
    src/core/i2c.cpp
    src/core/memory.cpp
    src/core/nand.cpp
    src/core/psp.cpp
    src/core/scheduler.cpp
    src/core/syscon.cpp
    src/core/systime.cpp
    src/core/allegrex/allegrex.cpp
    src/core/allegrex/cop0.cpp
    src/core/allegrex/fpu.cpp
    src/core/allegrex/interpreter.cpp
    src/core/allegrex/vfpu.cpp
    src/core/crypto/kirk.cpp
    src/core/crypto/spock.cpp
)

set(HEADERS
    src/common/file.hpp
    src/common/types.hpp
    src/core/ata.hpp
    src/core/ddr.hpp
    src/core/display.hpp
    src/core/dmacplus.hpp
    src/core/ge.hpp
    src/core/gpio.hpp
    src/core/hpremote.hpp
    src/core/intc.hpp
    src/core/i2c.hpp
    src/core/memory.hpp
    src/core/nand.hpp
    src/core/psp.hpp
    src/core/scheduler.hpp
    src/core/syscon.hpp
    src/core/systime.hpp
    src/core/allegrex/allegrex.hpp
    src/core/allegrex/cop0.hpp
    src/core/allegrex/fpu.hpp
    src/core/allegrex/interpreter.hpp
    src/core/allegrex/vfpu.hpp
    src/core/crypto/kirk.hpp
    src/core/crypto/spock.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE cryptopp)
