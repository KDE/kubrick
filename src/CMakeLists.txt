add_executable(kubrick)

target_sources(kubrick PRIVATE
    main.cpp 
    kubrick.cpp kubrick.h 
    game.cpp game.h 
    cube.cpp cube.h
    movetracker.cpp movetracker.h 
    quaternion.cpp quaternion.h
    gamedialog.cpp gamedialog.h 
    gameglview.cpp gameglview.h
    scenelabel.cpp scenelabel.h

    kubrick.qrc
)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX KUBRICK
    VERSION_HEADER kubrick_version.h
)

ecm_qt_declare_logging_category(kubrick
    HEADER kubrick_debug.h
    IDENTIFIER KUBRICK_LOG
    CATEGORY_NAME kubrick
    OLD_CATEGORY_NAMES LOG_KUBRICK
    DESCRIPTION "Kubrick game"
    EXPORT KUBRICK
)

file(GLOB ICONS_SRCS "*-apps-kubrick.png")
ecm_add_app_icon(kubrick ICONS ${ICONS_SRCS})

target_link_libraries(kubrick
    KDEGames6
    KF6::Crash
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::XmlGui
    Qt6::Svg
    Qt6::OpenGLWidgets
    OpenGL::GL
    OpenGL::GLU
)

if(APPLE)
    set_target_properties(kubrick PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME "Kubrick"
       MACOSX_BUNDLE_BUNDLE_NAME "Kubrick"
       MACOSX_BUNDLE_LONG_VERSION_STRING "Kubrick ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.kubrick"
       MACOSX_BUNDLE_COPYRIGHT "2024 The Kubrick Developers")
endif()


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
