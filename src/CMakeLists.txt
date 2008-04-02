
include_directories(${KDEGAMES_INCLUDE_DIR})
find_package(OpenGL REQUIRED)

########### next target ###############

set(kubrick_SRCS 
   main.cpp 
   kubrick.cpp 
   game.cpp 
   cube.cpp
   movetracker.cpp 
   quaternion.cpp
   gamedialog.cpp 
   gameglview.cpp )

kde4_add_executable(kubrick ${kubrick_SRCS})
kde4_add_app_icon(kubrick_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kubrick.png")

if(WIN32)
  set(opengl_libs opengl32.lib glu32.lib)
endif(WIN32)

target_link_libraries(kubrick
  ${QT_QTOPENGL_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEGAMES_LIBRARY}
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
)

install(TARGETS kubrick DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  kubrickui.rc DESTINATION ${DATA_INSTALL_DIR}/kubrick )
install( FILES  kubrick.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )

