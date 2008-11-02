
########### next target ###############

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
kde4_add_executable(kubrick ${kubrick_SRCS})

if(WIN32)
  set(opengl_libs opengl32.lib glu32.lib)
endif(WIN32)

target_link_libraries(kubrick
  ${QT_QTOPENGL_LIBRARY}
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KIO_LIBS}
  kdegames
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
)

install(TARGETS kubrick ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install( FILES  kubrickui.rc DESTINATION ${DATA_INSTALL_DIR}/kubrick )
install( FILES  kubrick.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )


