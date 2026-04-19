cmake_minimum_required(VERSION 3.16)
project(SmartScope)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Charts)

add_executable(SmartScope
    src/main.cpp
    src/MainWindow.cpp
    src/OscilloscopeWidget.cpp
    src/SmartUSBHub.cpp
)

target_link_libraries(SmartScope PRIVATE Qt6::Widgets Qt6::SerialPort Qt6::Charts)
