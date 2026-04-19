cmake_minimum_required(VERSION 3.20)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

project(CoreLab LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries on Windows" FORCE)
    set(SFML_USE_STATIC_STD_LIBS ON CACHE BOOL "Use static std libs" FORCE)
else()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries on Unix" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
    GIT_SHALLOW    ON
)

set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL_PKGCONFIG_FILES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

add_executable(CoreLab
    main.cpp
    app/App.cpp
    app/MachineController.cpp
    app/animation/MemoryReadAnimation.cpp
    app/core/Camera.cpp
    app/input/Controls.cpp
    app/scene/Scene.cpp
    app/sim/Cache.cpp
    app/sim/MemoryTransaction.cpp
    app/sim/RAM.cpp
    app/sim/simulation.cpp
    app/view/BusView.cpp
    app/view/BlockView.cpp
    app/view/rails/ArcRailSegment.cpp
    app/view/rails/BezierRailSegment.cpp
    app/view/CacheLineView.cpp
    app/view/CacheView.cpp
    app/view/CoreView.cpp
    app/view/CpuView.cpp
    app/view/PortView.cpp
    app/view/rails/RailBuilder.cpp
    app/view/rails/RailPath.cpp
    app/view/rails/RailSegment.cpp
    app/view/RamView.cpp
    app/view/ViewNode.cpp
    app/view/rails/StraightRailSegment.cpp
    app/MachineController.h
)

target_include_directories(CoreLab
    PRIVATE
        .
        app
)

target_link_libraries(CoreLab
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
)
