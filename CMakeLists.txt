cmake_minimum_required(VERSION 3.14)

project( fluidlite-qtmultimedia
         LANGUAGES C CXX
         VERSION 0.0.1
         DESCRIPTION "FluidLite Synth for Qt" )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED)
if((QT_VERSION VERSION_LESS 5.15) OR ((QT_VERSION_MAJOR EQUAL 6) AND (QT_VERSION VERSION_LESS 6.4)))
    message(FATAL_ERROR "Unsupported Qt Version ${QT_VERSION}")
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Multimedia REQUIRED)
find_package(Drumstick 2.6 COMPONENTS RT REQUIRED)

include(GNUInstallDirs)

add_subdirectory(FluidLite)
include(${PROJECT_BINARY_DIR}/FluidLite/fluidlite-targets.cmake)

add_subdirectory(libcommon)
add_subdirectory(cmdlnsynth)
add_subdirectory(guisynth)