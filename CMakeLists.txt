project (kubrick)

if(NOT COMPILING_KDEGAMES)
	find_package(KDE4 REQUIRED)
	include(KDE4Defaults)
	include(MacroLibrary)
	find_package(KDEGames REQUIRED)

	find_package(OpenGL REQUIRED)
	macro_log_feature(OPENGL_FOUND "OpenGL" "API for developing portable, interactive 2D and 3Dgraphics applications" "http://mesa3d.sourceforge.net" TRUE "" "Kubrick will not be built and KSudoku will not have Roxdoku support without OpenGL.")
	macro_log_feature(QT_QTOPENGL_FOUND "QtOpenGL" "Qt module that make it easy to use OpenGL in Qt apps" "http://doc.trolltech.com/latest/qtopengl.html" TRUE "" "Kubrick will not be built and KSudoku will not have Roxdoku support without QtOpenGL.")
	macro_display_feature_log()

	add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
	add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
	include_directories(${QDBUS_INCLUDE_DIRS} ${KDE4_INCLUDES} ${KDEGAMES_INCLUDE_DIRS})
endif(NOT COMPILING_KDEGAMES)

add_subdirectory (src)
add_subdirectory (data)
add_subdirectory (themes)
add_subdirectory (doc)
