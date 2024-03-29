cmake_minimum_required(VERSION 3.14)

project( fluidlite-qtmultimedia
         LANGUAGES C CXX
         VERSION 0.0.2
         DESCRIPTION "FluidLite Synth for Qt" )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED)
if ((CMAKE_SYSTEM_NAME MATCHES "Linux") AND (QT_VERSION_MAJOR EQUAL 6) AND (QT_VERSION VERSION_LESS 6.4))
    message(WARNING "Unsupported Qt version ${QT_VERSION} for system ${CMAKE_SYSTEM_NAME}")
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Multimedia REQUIRED)
find_package(Drumstick 2.6 COMPONENTS RT Widgets REQUIRED)

include(GNUInstallDirs)

find_package(fluidlite QUIET)
if (fluidlite_FOUND)
    message( STATUS "Using FluidLite v${fluidlite_VERSION}" )
else()
    add_subdirectory(FluidLite)
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/FluidLite/aliases.cmake)
        add_library(fluidlite::fluidlite-static ALIAS fluidlite-static)
    else()
        message(FATAL_ERROR "Missing file 'FluidLite/aliases.cmake'. Use: 'git submodule update'")
    endif()
endif()

add_subdirectory(libcommon)
add_subdirectory(cmdlnsynth)
add_subdirectory(guisynth)
