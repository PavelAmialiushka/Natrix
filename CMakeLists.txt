cmake_minimum_required(VERSION 3.15)

project(Natrix)

# conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} /W3 /WX /permissive- /wd4996 /wd4242")

#set(CMAKE_CXX_CLANG_TIDY clang-tidy;-checks=*;-warnings-as-errors=*)


set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")

SET(CMAKE_EXE_LINKER_FLAGS  
    "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")

# Qt5 library
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui PrintSupport Xml Network)
list(APPEND QT_LIBRARIES
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    Qt5::PrintSupport
    Qt5::Xml
    Qt5::Network
    )


add_subdirectory(src/geometry)
add_subdirectory(src/ui)

option(NATRIX_BUILD_TESTS "Build the tests." 0)
if(NATRIX_BUILD_TESTS)
    enable_testing()
    message(STATUS "Tests are enabled")

    include(GoogleTest)
    add_subdirectory(src/geometrytest)
endif()
