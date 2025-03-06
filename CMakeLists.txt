cmake_minimum_required(VERSION 3.16)
project(StorageHelper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Add Homebrew Qt path for macOS
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/qt/6.8.2")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt@6")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/filescanworker.cpp
    src/fileutils.cpp
)

set(HEADERS
    include/mainwindow.h
    include/filescanworker.h
    include/fileutils.h
)

set(UI_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/mainwindow.ui
)

add_executable(StorageHelper
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_include_directories(StorageHelper PRIVATE include)
target_include_directories(StorageHelper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(StorageHelper PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

if(WIN32)
    set_target_properties(StorageHelper PROPERTIES WIN32_EXECUTABLE TRUE)
endif() 