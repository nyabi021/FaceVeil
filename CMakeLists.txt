cmake_minimum_required(VERSION 3.24)

project(FaceVeil VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ONNXRUNTIME REQUIRED IMPORTED_TARGET libonnxruntime)

qt_add_executable(FaceVeil MACOSX_BUNDLE
    src/main.cpp
    src/MainWindow.cpp
    src/ImageScanner.cpp
    src/Mosaic.cpp
    src/ProcessorWorker.cpp
    src/ScrfdFaceDetector.cpp
    include/faceveil/FaceDetection.hpp
    include/faceveil/ImageScanner.hpp
    include/faceveil/MainWindow.hpp
    include/faceveil/Mosaic.hpp
    include/faceveil/ProcessorWorker.hpp
    include/faceveil/ScrfdFaceDetector.hpp
)

target_include_directories(FaceVeil PRIVATE include)
target_link_libraries(FaceVeil PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
    PkgConfig::ONNXRUNTIME
)

set_target_properties(FaceVeil PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.faceveil.app"
    MACOSX_BUNDLE_BUNDLE_NAME "FaceVeil"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)

install(TARGETS FaceVeil
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

add_executable(faceveil_inspect_model
    tools/inspect_model.cpp
)

target_link_libraries(faceveil_inspect_model PRIVATE PkgConfig::ONNXRUNTIME)
