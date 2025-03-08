cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

project(kubrick VERSION "1.1.${RELEASE_SERVICE_COMPACT_VERSION}")

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)

include(InternalMacros)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    OpenGLWidgets
    Svg
    Widgets
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DocTools
    I18n
    WidgetsAddons
    XmlGui
)

find_package(KDEGames6 6.0.0 REQUIRED)

find_package(OpenGL REQUIRED)

ecm_set_disabled_deprecation_versions(
    QT 6.9.0
    KF 6.12.0
    KDEGAMES 6.0
)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(themes)
add_subdirectory(doc)

ki18n_install(po)
kdoctools_install(po)

ecm_qt_install_logging_categories(EXPORT KUBRICK FILE kubrick.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
