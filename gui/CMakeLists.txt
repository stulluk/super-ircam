cmake_minimum_required(VERSION 3.16)
project(super_ircam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)

add_executable(super-ircam
  src/main.cpp
  src/MainWindow.cpp
  src/CaptureThread.cpp
  src/DeviceFind.cpp
  src/Colormap.cpp
)

target_include_directories(super-ircam PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(super-ircam PRIVATE Qt6::Widgets Qt6::Gui ${OpenCV_LIBS})

include(GNUInstallDirs)
install(TARGETS super-ircam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(PACKAGING_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/super-ircam")
install(FILES "${PACKAGING_ROOT}/usr/share/applications/super-ircam.desktop"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES "${PACKAGING_ROOT}/usr/share/icons/hicolor/128x128/apps/super-ircam.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps)
install(FILES "${PACKAGING_ROOT}/usr/share/icons/hicolor/256x256/apps/super-ircam.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
install(FILES "${PACKAGING_ROOT}/usr/share/icons/hicolor/scalable/apps/super-ircam.svg"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/flatpak/com.github.stulluk.SuperIRCam.metainfo.xml"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES "${PACKAGING_ROOT}/usr/share/icons/hicolor/128x128/apps/super-ircam.png"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps
        RENAME com.github.stulluk.SuperIRCam.png)
