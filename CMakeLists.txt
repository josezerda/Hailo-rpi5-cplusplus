cmake_minimum_required(VERSION 3.14)
project(hailo_detection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar OpenCV
find_package(OpenCV REQUIRED)

# Buscar GStreamer y sus módulos
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

link_directories(
    ${GSTREAMER_LIBRARY_DIRS}
    ${GLIB_LIBRARY_DIRS}
)

add_executable(hailo_detection src/main.cpp)

target_link_libraries(hailo_detection
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GLIB_LIBRARIES}
)