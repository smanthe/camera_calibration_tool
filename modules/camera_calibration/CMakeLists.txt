include(FetchContent)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_SHALLOW 1
    GIT_PROGRESS 1
    GIT_TAG v3.11.2)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

find_package(OpenCV REQUIRED)

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
    nlohmann_json::nlohmann_json
    Eigen3::Eigen)

target_include_directories(camcalib PUBLIC
    include
    ${OpenCV_INCLUDE_DIRS})
