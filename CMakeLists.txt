cmake_minimum_required(VERSION 3.16)
project(SmartScope)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SmartScope
src/main.cpp
src/MainWindow.cpp
src/OscilloscopeWidget.cpp
src/DataEngine.cpp
src/TriggerEngine.cpp
)

target_link_libraries(SmartScope Qt6::Widgets)
