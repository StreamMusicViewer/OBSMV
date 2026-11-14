cmake_minimum_required(VERSION 3.20)
project(obsmv VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Platform definitions
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# Locate LibObs
set(LIBOBS_FOUND FALSE)
find_package(libobs QUIET)
if(TARGET OBS::libobs)
    set(LIBOBS_FOUND TRUE)
    set(LIBOBS_TARGET OBS::libobs)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/libobs/obs.h")
    set(LIBOBS_FOUND TRUE)
    set(LIBOBS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/libobs")
    if(WIN32)
        set(LIBOBS_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/libobs/obs.lib")
    else()
        find_library(LIBOBS_LIB obs)
        if(LIBOBS_LIB)
            set(LIBOBS_LIBRARIES "${LIBOBS_LIB}")
        else()
            set(LIBOBS_LIBRARIES "")
        endif()
    endif()
else()
    message(FATAL_ERROR "libobs headers not found! Please install libobs-dev or provide deps/libobs.")
endif()

# Source files
set(OBSMV_SOURCES
    src/obsmv_plugin.cpp
    src/source/obsmv_source_combined.cpp
    src/source/obsmv_source_visualizer.cpp
    src/source/obsmv_source_nowplaying.cpp
    src/visualizer/audio_capture.cpp
    src/visualizer/fft_processor.cpp
    src/visualizer/visualizer_renderer.cpp
    src/render/text_renderer.cpp
    src/render/image_renderer.cpp
    src/render/card_renderer.cpp
    src/media/media_detector.cpp
    src/media/media_osmv_sync.cpp
    src/third_party/kissfft/kiss_fft.c
    src/third_party/kissfft/kiss_fftr.c
)

set(PLATFORM_LIBS "")

if(WIN32)
    list(APPEND OBSMV_SOURCES src/media/media_win_smtc.cpp)
    list(APPEND PLATFORM_LIBS windowsapp.lib runtimeobject.lib gdi32.lib)
elseif(APPLE)
    # macOS
    find_library(COCOA_LIB Cocoa REQUIRED)
    list(APPEND PLATFORM_LIBS ${COCOA_LIB})
else()
    # Linux
    list(APPEND OBSMV_SOURCES src/media/media_linux_mpris.cpp)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(DBUS dbus-1)
        if(DBUS_FOUND)
            list(APPEND PLATFORM_LIBS ${DBUS_LIBRARIES})
            include_directories(${DBUS_INCLUDE_DIRS})
        endif()
    endif()
    list(APPEND PLATFORM_LIBS m)
endif()

add_library(obsmv MODULE ${OBSMV_SOURCES})

# Target Include Directories
target_include_directories(obsmv PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/kissfft
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/json
)

if(TARGET OBS::libobs)
    target_link_libraries(obsmv PRIVATE OBS::libobs ${PLATFORM_LIBS})
else()
    target_include_directories(obsmv PRIVATE ${LIBOBS_INCLUDE_DIRS})
    if(LIBOBS_LIBRARIES)
        target_link_libraries(obsmv PRIVATE ${LIBOBS_LIBRARIES} ${PLATFORM_LIBS})
    else()
        target_link_libraries(obsmv PRIVATE ${PLATFORM_LIBS})
    endif()
endif()

if(APPLE)
    target_link_options(obsmv PRIVATE -undefined dynamic_lookup)
endif()

set_target_properties(obsmv PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obsmv"
)

# Optional automated installation to OBS Studio plugins directory if available
if(WIN32)
    set(OBS_PLUGIN_DIR "C:/Program Files/obs-studio/obs-plugins/64bit")
    set(OBS_DATA_DIR "C:/Program Files/obs-studio/data/obs-plugins/obsmv")
    
    install(TARGETS obsmv DESTINATION ${OBS_PLUGIN_DIR})
    install(DIRECTORY data/ DESTINATION ${OBS_DATA_DIR})
endif()

