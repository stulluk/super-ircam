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
