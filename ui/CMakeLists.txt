cmake_minimum_required(VERSION 3.5)

project(UI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE debug)

# set -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\mingw81_64\lib\cmake"
# in order to make find_package calls work...
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/5.15.2/mingw81_64/lib/cmake")
#link_directories(C:/Qt/5.15.2/mingw81_64/bin)

find_package(Qt5 CONFIG REQUIRED Core Widgets Gui OpenGL PrintSupport DataVisualization SerialPort Svg)
find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL PrintSupport SerialPort DataVisualization Svg REQUIRED)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/fftw/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/gl2ps/)

#aux_source_directory("C:/Qt/5.15.2/Src" QT_SOURCE_FILES)
include_directories("C:/Qt/5.15.2/Src/qtbase/src")
include_directories("C:/Qt/5.15.2/Src/qt3d/src")
include_directories("C:/Qt/5.15.2/Src/qtsvg/src")
include_directories("C:/Qt/5.15.2/Src/qtserialport/src")
include_directories("C:/Qt/5.15.2/Src/qtdatavis3d/src/datavisualization")

add_compile_options(-mavx -msse -msse2 -msse3 -msse4 -msse4.1 -msse4.2 -march=native -ffast-math) #-O3

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        SerialThread.h
        SerialThread.cpp
        fwht/fwht.h
        resources/resources.qrc constants.h
        fftw/fftw3.h SvgIconEngine.cpp SvgIconEngine.h)

set(QCustomPlot_SOURCES
        qcp/qcustomplot.cpp
        qcp/qcustomplot.h)

add_library(QCustomPlot SHARED ${QCustomPlot_SOURCES})
add_executable(UI ${PROJECT_SOURCES})
add_executable(FWHT_Performance tests/fwht_performance.cpp)
add_executable(DataReader tests/data_read_performance.cpp)
#        fwht/hadamard_unrolling.cpp
#        fwht/hadamard_avx.cpp
#        fwht/hadamard_openmp.cpp
#        fwht/hadamard_pthreads.cpp)



target_link_libraries(FWHT_Performance PRIVATE
        Qt5::Widgets
        Qt5::Core)
target_link_libraries(DataReader PRIVATE
        Qt5::Widgets
        Qt5::Core)

target_link_libraries(UI PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        QCustomPlot
        Qt5::SerialPort
        Qt5::OpenGL
        Qt5::DataVisualization
        Qt5::Svg
        fftw3-3)
target_link_libraries(QCustomPlot
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::PrintSupport
        Qt5::OpenGL
        OpenGL32)

target_compile_definitions(QCustomPlot PUBLIC -DQCUSTOMPLOT_USE_OPENGL)

include_directories(C:/Qt/5.15.2/mingw81_64/include)
#include_directories(C:/Qt/5.15.2/mingw81_64/include/Qt)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/fftw/)

add_custom_command(TARGET UI POST_BUILD
        COMMAND C:/Qt/5.15.2/mingw81_64/bin/windeployqt ${CMAKE_CURRENT_BINARY_DIR}/UI.exe
)

add_custom_command(TARGET UI POST_BUILD
        COMMAND C:/Qt/5.15.2/mingw81_64/bin/windeployqt --debug ${CMAKE_CURRENT_BINARY_DIR}/libQCustomPlot.dll
        )

add_custom_command(TARGET UI POST_BUILD  # make sure fftw library is available...
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/fftw/libfftw3-3.dll"
        $<TARGET_FILE_DIR:UI>)

add_custom_command(TARGET UI POST_BUILD  # make sure fftw library is available...
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/gl2ps/libgl2ps.dll"
        $<TARGET_FILE_DIR:UI>)

add_custom_command(TARGET DataReader POST_BUILD
        COMMAND C:/Qt/5.15.2/mingw81_64/bin/windeployqt ${CMAKE_CURRENT_BINARY_DIR}/DataReader.exe
        )

add_custom_command(TARGET FWHT_Performance POST_BUILD
        COMMAND C:/Qt/5.15.2/mingw81_64/bin/windeployqt ${CMAKE_CURRENT_BINARY_DIR}/FWHT_Performance.exe
        )

file(GLOB CleanDlls ${CMAKE_CURRENT_BINARY_DIR}/*.dll)
set_property(
        TARGET UI
        APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES ${CleanDlls}
)

add_dependencies(UI QCustomPlot)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UI)
endif()
