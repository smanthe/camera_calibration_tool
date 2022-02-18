cmake_minimum_required(VERSION 3.6.2 FATAL_ERROR)
project(cameraCalibrationTool)

  # Find OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
set(LIBS ${LIBS} ${OpenCV_LIBRARIES})

  # Find Eigen
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLUDE_DIRS})

set(SOURCE_FILES
    src/CameraCalibration.cpp
    src/utils.cpp)

add_library(camcalib
    ${SOURCE_FILES})

set_target_properties(camcalib PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)

target_link_libraries(camcalib PUBLIC
    ${LIBS})

target_include_directories(camcalib PUBLIC
    include)

add_subdirectory(./modules/gui)
