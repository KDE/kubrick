
include_directories(${KDEGAMES_INCLUDE_DIR})

########### next target ###############

set(kubrick_SRCS 
   main.cpp 
   kubrick.cpp 
   game.cpp 
   cube.cpp
   movetracker.cpp 
   gamedialog.cpp 
   gameglview.cpp )

kde4_add_executable(kubrick ${kubrick_SRCS})
kde4_add_app_icon(kubrick_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kubrick.png")

target_link_libraries(kubrick ${QT_QTOPENGL_LIBRARY}
                              ${KDE4_KDEUI_LIBS} ${KDEGAMES_LIBRARY})

install(TARGETS kubrick DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES  kubrickui.rc DESTINATION ${DATA_INSTALL_DIR}/kubrick )
install( FILES  kubrick.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )

