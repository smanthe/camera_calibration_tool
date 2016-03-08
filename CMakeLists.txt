cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project(cameraCalibrationTool)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

  # Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBRARIES})

  # Find Boost
find_package(Boost COMPONENTS filesystem system)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

  # Find Eigen
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLUDE_DIRS})

find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Concurrent REQUIRED)
set(LIBS ${LIBS} Qt5::Widgets Qt5::OpenGL Qt5::Concurrent)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(CAMCALIB_MOC ${CMAKE_CURRENT_SOURCE_DIR}/gui/include/QImageModel.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/gui/include/QProgressState.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/gui/include/QProgressState.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/gui/include/ResizeableGraphicsView.h
                          ${CMAKE_CURRENT_SOURCE_DIR}/gui/include/CalibrationWidget.h)

add_executable(camcalib src/CameraCalibration.cpp
                        src/qtOpenCVConversions.cpp
                        src/utils.cpp
                        gui/src/CalibrationWidget.cpp
                        gui/src/main.cpp
                        gui/src/QFileItem.cpp
                        gui/src/QImageModel.cpp
                        gui/src/QProgressState.cpp
                        gui/src/ResizeableGraphicsView.cpp
                        ${CAMCALIB_MOC})

target_include_directories(camcalib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(camcalib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gui/include)

set_property(TARGET camcalib PROPERTY CXX_STANDARD 11)
set_property(TARGET camcalib PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(camcalib ${LIBS})