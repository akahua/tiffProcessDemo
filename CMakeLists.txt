cmake_minimum_required(VERSION 3.16)

project(tiffProcessDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCV_DIR "D:/thirdparty/opencv_new/build/install/lib")
set(CMAKE_PREFIX_PATH "D:/Qt/6.9.0/msvc2022_64/lib/cmake")  # Qt 安装路径
list(APPEND CMAKE_PREFIX_PATH "D:/thirdparty/json/install")
list(APPEND CMAKE_PREFIX_PATH "D:/thirdparty/glog/build/install")
list(APPEND CMAKE_PREFIX_PATH "D:/thirdparty/libtiff/install")

find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(glog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tiff CONFIG REQUIRED)

set(TIFF_INSTALL_DIR ${CMAKE_SOURCE_DIR}/build/TiffProcessLibrary)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

set(LIB_SOURCES
    tiffprocesslibrary.h
    tiffprocesslibrary.cpp
)

add_library(TiffProcessLibrary SHARED
    ${LIB_SOURCES}
)
target_compile_definitions(TiffProcessLibrary
    PRIVATE TIFFPROCESSLIBRARY_EXPORTS
)
install(TARGETS TiffProcessLibrary
    RUNTIME DESTINATION ${TIFF_INSTALL_DIR}
    ARCHIVE DESTINATION ${TIFF_INSTALL_DIR}
    LIBRARY DESTINATION ${TIFF_INSTALL_DIR}
)

install(FILES
    tiffprocesslibrary.h
    DESTINATION ${TIFF_INSTALL_DIR}/include
)




qt_add_executable(tiffProcessDemo
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    imageview.h
    imageview.cpp
    controlpanel.h
    controlpanel.cpp
    tiffprocess.h
    tiffprocess.cpp
    tiffprocessapi.h
    tiffprocessapi.cpp
    utils.h
    utils.cpp
    tiffimage.h
    tiffimage.cpp
    pstemplate.h pstemplate.cpp
)
target_link_libraries(TiffProcessLibrary PRIVATE
    ${OpenCV_LIBS}
    TIFF::tiff
)

target_link_libraries(tiffProcessDemo PRIVATE
    Qt6::Widgets
    ${OpenCV_LIBS}
    glog::glog
    nlohmann_json::nlohmann_json
    TIFF::tiff
)

set_target_properties(tiffProcessDemo PROPERTIES
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(tiffProcessDemo)
