
########### next target ###############

include_directories( ${OPENGL_INCLUDE_DIR} )

set(kubrick_SRCS 
   main.cpp 
   kubrick.cpp 
   game.cpp 
   cube.cpp
   movetracker.cpp 
   quaternion.cpp
   gamedialog.cpp 
   gameglview.cpp
   scenelabel.cpp )

kde4_add_app_icon(kubrick_SRCS "hi*-app-kubrick.png")
add_executable(kubrick ${kubrick_SRCS})

if(WIN32)
  set(opengl_libs opengl32.lib glu32.lib)
endif(WIN32)

target_link_libraries(kubrick
  Qt5::OpenGL
  Qt5::Svg
  KF5::KDELibs4Support
  KF5::KIOCore
  KF5KDEGames
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
)

install(TARGETS kubrick ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install( FILES  kubrickui.rc DESTINATION ${DATA_INSTALL_DIR}/kubrick )
install( PROGRAMS  kubrick.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

ecm_install_icons( ${ICON_INSTALL_DIR} )


