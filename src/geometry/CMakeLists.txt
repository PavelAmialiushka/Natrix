cmake_minimum_required(VERSION 3.15)

project (geometry)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)

FILE(GLOB sources ./*.cpp)
FILE(GLOB headers ./*.h)

add_library(${PROJECT_NAME} ${sources} ${headers})
add_library(sub::geometry ALIAS ${PROJECT_NAME})

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/..
)

target_link_libraries( ${PROJECT_NAME}
    ${QT_LIBRARIES}
    )
