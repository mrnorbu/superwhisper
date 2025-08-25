cmake_minimum_required(VERSION 3.20)
project(SuperWhisper)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable optimizations for Apple Silicon ONLY (Maximum Performance)
if(APPLE)
    # Build for Apple Silicon only for maximum speed
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    
    # Aggressive Apple Silicon optimizations for MAXIMUM SPEED
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -ffast-math -funroll-loops -mcpu=apple-m1 -mtune=apple-m1")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG -ffast-math -funroll-loops -mcpu=apple-m1 -mtune=apple-m1")
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
endif()

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 REQUIRED)

# PortAudio - use pkg-config to find it
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# Dear ImGui - build from source
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
if(NOT EXISTS "${IMGUI_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Dear ImGui CMakeLists.txt not found. Run ./build.sh to download dependencies.")
endif()
add_subdirectory(external/imgui)

# whisper.cpp - build from source with MAXIMUM PERFORMANCE
set(WHISPER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/whisper.cpp")
if(NOT EXISTS "${WHISPER_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "whisper.cpp CMakeLists.txt not found. Run ./build.sh to download dependencies.")
endif()

# CRITICAL: Enable all performance optimizations for whisper.cpp
set(WHISPER_METAL ON CACHE BOOL "Enable Metal GPU acceleration on Apple Silicon")
set(WHISPER_ACCELERATE ON CACHE BOOL "Enable Accelerate framework for Intel Macs")
set(WHISPER_OPENBLAS OFF CACHE BOOL "Disable OpenBLAS to avoid conflicts")
set(WHISPER_CUDA OFF CACHE BOOL "Disable CUDA (not available on macOS)")
set(WHISPER_CUBLAS OFF CACHE BOOL "Disable cuBLAS")
set(WHISPER_CLBLAST OFF CACHE BOOL "Disable CLBlast")
set(WHISPER_HIPBLAS OFF CACHE BOOL "Disable hipBLAS")

# Enable SIMD optimizations
set(GGML_NATIVE ON CACHE BOOL "Enable native optimizations")
set(GGML_LTO ON CACHE BOOL "Enable Link Time Optimization")

add_subdirectory(external/whisper.cpp)

# Set source files
set(SOURCES
    src/main.cpp
    src/audio_recorder.cpp
    src/whisper_wrapper.cpp
    src/gui_manager.cpp
    src/hotkey_manager.cpp
    src/settings.cpp
)

# Create executable
add_executable(SuperWhisper ${SOURCES})

# Include directories
target_include_directories(SuperWhisper PRIVATE
    src/
    external/imgui
    external/whisper.cpp
    ${PORTAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(SuperWhisper PRIVATE
    imgui
    whisper
    glfw
    ${PORTAUDIO_LIBRARIES}
    OpenGL::GL
    ${CMAKE_DL_LIBS}
)

# Link directories
target_link_directories(SuperWhisper PRIVATE ${PORTAUDIO_LIBRARY_DIRS})

# Compiler flags
target_compile_options(SuperWhisper PRIVATE ${PORTAUDIO_CFLAGS_OTHER})

# macOS specific settings
if(APPLE)
    target_link_libraries(SuperWhisper PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Carbon"
    )
    
    # Set bundle properties
    set_target_properties(SuperWhisper PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    )
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(SuperWhisper PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()
