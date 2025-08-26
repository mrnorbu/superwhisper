cmake_minimum_required(VERSION 3.20)
project(SuperWhisperCLI)

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

# PortAudio - use pkg-config to find it
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

# nlohmann/json - header-only library
find_package(nlohmann_json 3.2.0 REQUIRED)

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

# Set source files for CLI version
set(SOURCES
    src/cli_main.cpp
    src/audio_recorder.cpp
    src/whisper_wrapper.cpp
    src/settings.cpp
    src/hotkey_manager.cpp
)

# Create executable
add_executable(SuperWhisperCLI ${SOURCES})

# Include directories
target_include_directories(SuperWhisperCLI PRIVATE
    src/
    external/whisper.cpp
    ${PORTAUDIO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(SuperWhisperCLI PRIVATE
    whisper
    ${PORTAUDIO_LIBRARIES}
    ${CMAKE_DL_LIBS}
    nlohmann_json::nlohmann_json
)

# Link directories
target_link_directories(SuperWhisperCLI PRIVATE ${PORTAUDIO_LIBRARY_DIRS})

# Compiler flags
target_compile_options(SuperWhisperCLI PRIVATE ${PORTAUDIO_CFLAGS_OTHER})

# macOS specific settings
if(APPLE)
    target_link_libraries(SuperWhisperCLI PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework Carbon"
    )
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(SuperWhisperCLI PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()
