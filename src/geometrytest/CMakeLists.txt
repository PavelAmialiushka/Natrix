cmake_minimum_required(VERSION 3.15)

project (geometrytests)

FILE(GLOB sources ./*.cpp)
FILE(GLOB headers ./*.h)
add_executable(${PROJECT_NAME} 
    ${sources}
    ${headers}
)

target_link_libraries(${PROJECT_NAME} 
    ${QT_LIBRARIES}
    sub::geometry
    ${CONAN_LIBS}
)

#gtest_discover_tests(${PROJECT_NAME})
gtest_add_tests(${PROJECT_NAME} "" AUTO)
