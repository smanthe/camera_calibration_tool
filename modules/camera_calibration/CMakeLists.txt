find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(Eigen3 REQUIRED)

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
    ${OpenCV_LIBRARIES}
    Eigen3::Eigen)

target_include_directories(camcalib PUBLIC
    include
    extern/json/include)
