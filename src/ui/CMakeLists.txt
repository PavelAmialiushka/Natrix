cmake_minimum_required(VERSION 3.15)

project(ui)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# There may be a way to tell up front if Qt5 is going to be found, but I haven't found
# a foolproof way to do it yet, so settle for the default error message for now.
#if(NOT CMAKE_PREFIX_PATH AND NOT Qt5Widgets_DIR)
#    message(WARNING "CMAKE_PREFIX_PATH is not defined, so find_package may not work. Set the CMAKE_PREFIX_PATH "
#            "environment variable to the install prefix of Qt 5, either on the command line as "
#            "-DCMAKE_PREFIX_PATH=\"path/to/Qt5/lib/cmake\" or with set(CMAKE_PREFIX_PATH path/to/Qt5/lib/cmake)")
#endif(NOT CMAKE_PREFIX_PATH AND NOT Qt5Widgets_DIR)

# Make this a GUI application on Windows
if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

FILE(GLOB sources ./*.cpp)
FILE(GLOB ui_files ./*.ui)
FILE(GLOB heaaders ./*.h)

# Tell CMake to create the helloworld executable
add_executable( ${PROJECT_NAME}
       ${sources}
       ${ui_files}
       ${headers}
       mainWindow.qrc)

# Add the Qt5 Widgets for linking
target_link_libraries( ${PROJECT_NAME}
    sub::geometry
    ${QT_LIBRARIES}
    ${CONAN_LIBS}
    )
