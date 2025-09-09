cmake_minimum_required(VERSION 3.6)

project(SeismicAmptune VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Set Qt5 to use MOC automatically
set(CMAKE_AUTOMOC ON)

# --- Source files ---
set(IOUTILS_SOURCES
    src/ioutils/segy_reader.cpp
    src/ioutils/segy_writer.cpp
)

set(AMPLIFY_SOURCES
    src/amplify/amplify.cpp
)

set(GUI_SOURCES
    src/gui/seismic_canvas.cpp
    src/gui/seismic_app.cpp
)

set(MAIN_SOURCES
    main.cpp
)

# --- Create libraries ---
add_library(ioutils_lib STATIC ${IOUTILS_SOURCES})
add_library(amplify_lib STATIC ${AMPLIFY_SOURCES})

# MODERN CMAKE: Specify header paths for each target individually.
# PUBLIC means that both the library itself and everything that links with it
# will see this path. This is exactly what we need.
target_include_directories(ioutils_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(amplify_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# MODERN CMAKE: Removed unnecessary linking of libraries with Qt5::Core.
# They don't depend on Qt.

# --- Create executable ---
add_executable(seismic_amptune ${MAIN_SOURCES} ${GUI_SOURCES})

# MODERN CMAKE: The executable needs access to headers from src
target_include_directories(seismic_amptune PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link libraries with executable
target_link_libraries(seismic_amptune 
    PRIVATE # PRIVATE, as this is the final product
    ioutils_lib 
    amplify_lib 
    Qt5::Core 
    Qt5::Widgets
)

# Set target properties 
set_target_properties(seismic_amptune PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Installation 
install(TARGETS seismic_amptune
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# --- Compiler options ---

# General warning flags
if(MSVC)
    target_compile_options(seismic_amptune PRIVATE /W4)
else()
    target_compile_options(seismic_amptune PRIVATE -Wall -Wextra -Wpedantic)
endif()

# MODERN CMAKE: Use generator expressions for configuration-dependent options
target_compile_definitions(seismic_amptune PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)
target_compile_options(seismic_amptune PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)


# Print configuration info
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt5 version: ${Qt5_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
