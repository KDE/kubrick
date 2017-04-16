
########### next target ###############

include_directories( ${OPENGL_INCLUDE_DIR} )

set(kubrick_SRCS 
    kubrick_debug.cpp
    main.cpp 
    kubrick.cpp 
    game.cpp 
    cube.cpp
    movetracker.cpp 
    quaternion.cpp
    gamedialog.cpp 
    gameglview.cpp
    scenelabel.cpp
)

file(GLOB ICONS_SRCS "*-apps-kubrick.png")
ecm_add_app_icon(kubrick_SRCS ICONS ${ICONS_SRCS})
add_executable(kubrick ${kubrick_SRCS})

if(WIN32)
  set(opengl_libs opengl32.lib glu32.lib)
endif(WIN32)

target_link_libraries(kubrick
    Qt5::OpenGL
    Qt5::Svg
    KF5::Crash
    KF5::KDELibs4Support
    KF5::KIOCore
    KF5KDEGames
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
)

install(TARGETS kubrick ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES  kubrickui.rc DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kubrick)
install(PROGRAMS  org.kde.kubrick.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kubrick.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ecm_install_icons(ICONS
    128-apps-kubrick.png
    16-apps-kubrick.png
    22-apps-kubrick.png
    32-apps-kubrick.png
    48-apps-kubrick.png
    64-apps-kubrick.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)

