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

if(WIN32)
    set(ONNXRUNTIME_ROOT "" CACHE PATH "Path to the ONNX Runtime package root")
    find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        HINTS
            "${ONNXRUNTIME_ROOT}/include"
            "$ENV{ONNXRUNTIME_ROOT}/include"
    )
    find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        HINTS
            "${ONNXRUNTIME_ROOT}/lib"
            "$ENV{ONNXRUNTIME_ROOT}/lib"
    )

    if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIBRARY)
        message(FATAL_ERROR "ONNX Runtime was not found. Configure with -DONNXRUNTIME_ROOT=C:/path/to/onnxruntime-win-x64")
    endif()

    add_library(onnxruntime::onnxruntime UNKNOWN IMPORTED)
    set_target_properties(onnxruntime::onnxruntime PROPERTIES
        IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ONNXRUNTIME REQUIRED IMPORTED_TARGET libonnxruntime)
    add_library(onnxruntime::onnxruntime ALIAS PkgConfig::ONNXRUNTIME)
endif()

set(FACEVEIL_APP_OPTIONS)
if(APPLE)
    list(APPEND FACEVEIL_APP_OPTIONS MACOSX_BUNDLE)
elseif(WIN32)
    list(APPEND FACEVEIL_APP_OPTIONS WIN32)
endif()

qt_add_executable(FaceVeil ${FACEVEIL_APP_OPTIONS}
    src/main.cpp
    src/MainWindow.cpp
    src/ImageScanner.cpp
    src/Mosaic.cpp
    src/ProcessorWorker.cpp
    src/ReviewDialog.cpp
    src/ScrfdFaceDetector.cpp
    include/faceveil/FaceDetection.hpp
    include/faceveil/ImageScanner.hpp
    include/faceveil/MainWindow.hpp
    include/faceveil/Mosaic.hpp
    include/faceveil/ProcessorWorker.hpp
    include/faceveil/ReviewDialog.hpp
    include/faceveil/ReviewTypes.hpp
    include/faceveil/ScrfdFaceDetector.hpp
)

if(WIN32)
    target_sources(FaceVeil PRIVATE assets/FaceVeil.rc)
endif()

target_include_directories(FaceVeil PRIVATE include)
target_link_libraries(FaceVeil PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
    onnxruntime::onnxruntime
)

if(APPLE)
    set_target_properties(FaceVeil PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.faceveil.app"
        MACOSX_BUNDLE_BUNDLE_NAME "FaceVeil"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
endif()

install(TARGETS FaceVeil
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

add_executable(faceveil_inspect_model
    tools/inspect_model.cpp
)

target_link_libraries(faceveil_inspect_model PRIVATE onnxruntime::onnxruntime)
