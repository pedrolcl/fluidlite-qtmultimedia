set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set( HEADERS
    programsettings.h
    synthcontroller.h
    synthrenderer.h
)

set( SOURCES
    programsettings.cpp
    synthcontroller.cpp 
    synthrenderer.cpp
)

add_library( fluidlite-libcommon SHARED ${HEADERS} ${SOURCES} )

set_target_properties( fluidlite-libcommon PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries( fluidlite-libcommon 
    PUBLIC
        fluidlite::fluidlite-static
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Multimedia
    PRIVATE
        Drumstick::RT
)

target_include_directories( fluidlite-libcommon
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions( fluidlite-libcommon PRIVATE
    VERSION=${PROJECT_VERSION}
    $<$<CONFIG:RELEASE>:QT_NO_DEBUG_OUTPUT>
)

install( TARGETS fluidlite-libcommon
         DESTINATION ${CMAKE_INSTALL_LIBDIR} )

install ( FILES ${HEADERS}
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
