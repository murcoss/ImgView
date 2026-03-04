cmake_minimum_required(VERSION 3.21)
project(ImgViewer LANGUAGES CXX)


# Export compile commands for clangd / VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable multi-core compilation
if(MSVC)
    add_compile_options(/MP)
endif()

# Find Qt (adjust version as needed)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg Sql)

# Or use Qt5 (uncomment this and comment out the Qt6 line above if using Qt5)
# find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Concurrent Svg)

# Sources
set(SOURCES
    IconEngine.cpp
    ImgView.cpp
    ImageLoaderTask.cpp
    ImageHashStore.cpp
    ImageItem.cpp
    ImageLoaderQueue.cpp
    DirIteratorTask.cpp
    main.cpp
)

# Headers (for MOC)
set(HEADERS
    ImgView.h
    MainWindow.h
    ImageLoaderTask.h
    ImageHashStore.h
    ImageLoaderQueue.h
    DirIteratorTask.h
    main.cpp
)

# Resources
set(RESOURCES
    Resource.rc
    ImgViewLogo.ico
)

# Define the executable
add_executable(ImgViewer
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Windows subsystem (no console window)
if(WIN32)
    set_target_properties(ImgViewer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Include Qt modules
target_link_libraries(ImgViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Svg
    Qt6::Sql
)

# Add include directory if needed
target_include_directories(ImgViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable AUTOMOC/ AUTOUIC/ AUTORCC
set_target_properties(ImgViewer PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Remove GCC-only flags when using clangd
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(REPLACE "-mno-direct-extern-access" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()