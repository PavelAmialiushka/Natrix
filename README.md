# Natrix

Pipe drawing tool

For more information please visit https://natrixlabs.ru

You can ask question at [forum](https://natrixlabs.ru/forums/)

Or contact author: https://telegram.me/RedApe

# How to build

## Prerequisites

To build Natrix you need 
1. Microsoft Visual Studio 2019
   - https://visualstudio.microsoft.com/downloads/
2. Qt 5.15.2 installed
   - Download online installer https://www.qt.io/download-qt-installer
   - Install 5.15.2 x64 version for MSVS_2019
3. Python3 installed
   - https://www.python.org/downloads/
4. CMake installed
   - https://cmake.org/download/
5. Conan installed
   - https://docs.conan.io/en/latest/installation.html
6. Makensis installed (if you need to create installer)
   - https://nsis.sourceforge.io/Download

## Build

### Release version

Run 
> build_installer.cmd

Then you can find results at `result/` folder

### Debug version

Run 
>build_debug.cmd

Than open build-debug\Natrix.sln
    
