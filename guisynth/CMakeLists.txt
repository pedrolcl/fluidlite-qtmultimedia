set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set( SOURCES main.cpp mainwindow.cpp customcombobox.h customcombobox.cpp)
set( HEADERS mainwindow.h )
set( FORMS mainwindow.ui )
set( RESOURCES guisynth.qrc )

add_executable( fluidlite-guisynth
  ${SOURCES}
  ${HEADERS}
  ${FORMS}
  ${RESOURCES}
)

target_link_libraries( fluidlite-guisynth PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Drumstick::RT
  Drumstick::Widgets
  fluidlite-libcommon
)

target_compile_definitions( fluidlite-guisynth PRIVATE
    VERSION=${PROJECT_VERSION}
    $<$<CONFIG:RELEASE>:QT_NO_DEBUG_OUTPUT>
)

install( TARGETS fluidlite-guisynth
         DESTINATION ${CMAKE_INSTALL_BINDIR} )

install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/fluidlite-guisynth.desktop
         DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
         RENAME fluidlite-qt.desktop )
