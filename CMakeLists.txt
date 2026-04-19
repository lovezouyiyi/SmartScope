cmake_minimum_required(VERSION 3.16)
project(SmartScope)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 尝试查找 Qt6 Charts，如果找不到则使用 Widgets 和 SerialPort
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)
find_package(Qt6 COMPONENTS Charts QUIET)

add_executable(SmartScope
    src/main.cpp
    src/MainWindow.cpp
    src/OscilloscopeWidget.cpp
    src/SmartUSBHub.cpp
)

if(Qt6Charts_FOUND)
    target_link_libraries(SmartScope PRIVATE Qt6::Widgets Qt6::SerialPort Qt6::Charts)
    target_compile_definitions(SmartScope PRIVATE USE_QT_CHARTS)
    message(STATUS "Qt6 Charts found - using Qt Charts for oscilloscope")
else()
    target_link_libraries(SmartScope PRIVATE Qt6::Widgets Qt6::SerialPort)
    message(WARNING "Qt6 Charts not found - using QPainter for oscilloscope (install Qt6 Charts for better visualization)")
endif()
