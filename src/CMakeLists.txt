add_executable(kubrick)

target_sources(kubrick PRIVATE
    main.cpp 
    kubrick.cpp 
    game.cpp 
    cube.cpp
    movetracker.cpp 
    quaternion.cpp
    gamedialog.cpp 
    gameglview.cpp
    scenelabel.cpp

    kubrick.qrc
)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KUBRICK
    VERSION_HEADER kubrick_version.h
)

ecm_qt_declare_logging_category(kubrick
    HEADER kubrick_debug.h
    IDENTIFIER KUBRICK_LOG
    CATEGORY_NAME LOG_KUBRICK
    DESCRIPTION "kdegames (kubrick)"
    EXPORT KUBRICK
)

file(GLOB ICONS_SRCS "*-apps-kubrick.png")
ecm_add_app_icon(kubrick ICONS ${ICONS_SRCS})

target_link_libraries(kubrick
    KDEGames6
    KF6::Crash
    KF6::ConfigCore
    KF6::KIOCore
    KF6::XmlGui
    Qt6::Svg
    Qt6::OpenGLWidgets
    OpenGL::GL
    OpenGL::GLU
)

install(TARGETS kubrick ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

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
