cmake_minimum_required(VERSION 3.14)
project(scan_adaptivedenoising LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    adaptivedenoising.cpp
    adaptivedenoisingblockqueue.cpp
    adaptivedenoisingprocessor.cpp
    adaptivedenoising_global.cpp
)

set(HEADERS
    adaptivedenoising.h
    adaptivedenoisingblockqueue.h
    adaptivedenoisingprocessor.h
    adaptivedenoising_global.h
)

set(FILE_TO_UPDATE adaptivedenoising_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_utils
    mne_fiff
    mne_rtprocessing
    eigen
    scShared
    scMeas
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SCAN_ADAPTIVEDENOISING_PLUGIN
    MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
    MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}"
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
