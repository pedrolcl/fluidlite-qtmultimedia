add_executable( fluidlite-cmdlnsynth main.cpp )

target_link_libraries( fluidlite-cmdlnsynth
    Qt${QT_VERSION_MAJOR}::Core
    Drumstick::RT
    fluidlite-libcommon
)

target_compile_definitions( fluidlite-cmdlnsynth PRIVATE
    VERSION=${PROJECT_VERSION}
    $<$<CONFIG:RELEASE>:QT_NO_DEBUG_OUTPUT>
)

install( TARGETS fluidlite-cmdlnsynth 
         DESTINATION ${CMAKE_INSTALL_BINDIR} )
