# Find the QtWidgets library
find_package(Qt5 COMPONENTS Widgets Concurrent REQUIRED)

set(LIBS Qt5::Widgets Qt5::Concurrent)

set(CALIBGUI_HEADER_FILES
    include/ImageModel.h
    include/qtOpenCVConversions.h
    include/ProgressState.h
    include/ResizeableGraphicsView.h
    include/CalibrationWidget.h)

set(CALIBGUI_SRC_FILES
    src/CalibrationWidget.cpp
    src/main.cpp
    src/FileItem.cpp
    src/ImageModel.cpp
    src/qtOpenCVConversions.cpp
    src/ProgressState.cpp
    src/ResizeableGraphicsView.cpp)

set(CALIBGUI_FORM_FILES
    forms/CalibrationWidget.ui)

qt5_wrap_cpp(CALIBGUI_MOC ${CALIBGUI_HEADER_FILES})
qt5_wrap_ui(CALIBGUI_FORMS_HEADERS ${CALIBGUI_FORM_FILES})

add_executable(calibGui ${CALIBGUI_SRC_FILES}
                        ${CALIBGUI_MOC}
                        ${CALIBGUI_FORMS_HEADERS})

target_include_directories(calibGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(calibGui
    ${LIBS}
    camcalib)

set_target_properties(calibGui PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO)
