# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(sdr_project VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set specific compiler flags for the Debug build type
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")

# Get the current date in YYYYMMDD format
string(TIMESTAMP CURRENT_DATE "%Y%m%d" UTC)

# Configure the config.h file from the template in the config folder
configure_file(${CMAKE_SOURCE_DIR}/config/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h @ONLY)

# Specify the directories where headers and source files are located
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add source files
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

# Add executable
add_executable(qs1r_server ${SOURCES})

# Define __LINUX_ALSA__ for the target
target_compile_definitions(qs1r_server PRIVATE __LINUX_ALSA__)
# Undefine flags using compiler options
target_compile_options(qs1r_server PRIVATE -U__RTAUDIO_DUMMY__ -U__IIR_NOTCH__ -U__NOISE_BLANKERS__ -U__AUTO_NOTCH__ -U__BINAURAL__)
# Define and undefine flags as needed
target_compile_definitions(qs1r_server PRIVATE __DAC_OUT__)
target_compile_options(qs1r_server PRIVATE -U__SOUND_OUT__)

# Specify the build output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)

# Set build type to Debug
set(CMAKE_BUILD_TYPE Debug)

# Find required packages
find_package(PkgConfig REQUIRED)

# Check for required libraries
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
pkg_check_modules(LIBSAMPLERATE REQUIRED samplerate)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(READLINE REQUIRED readline)

# Check for RtAudio (use explicit path fallback if pkg-config fails)
pkg_check_modules(RTAUDIO rtaudio)
if (NOT RTAUDIO_FOUND)
    message(WARNING "RtAudio not found with pkg-config. Adding fallback path.")
    set(RTAUDIO_INCLUDE_DIRS "/usr/include/rtaudio")
    set(RTAUDIO_LIBRARIES "/usr/lib/librtaudio.so")
endif()

# Include directories for the found packages
include_directories(
    ${LIBUSB_INCLUDE_DIRS}
    ${LIBSAMPLERATE_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${READLINE_INCLUDE_DIRS}
    ${RTAUDIO_INCLUDE_DIRS}
)

# Link libraries
link_directories(${LIBUSB_LIBRARY_DIRS})
target_link_libraries(qs1r_server
    ${LIBUSB_LIBRARIES}
    ${LIBSAMPLERATE_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${READLINE_LIBRARIES}
    ${RTAUDIO_LIBRARIES}
)

# Add messages for debugging purposes
message(STATUS "Found libusb: ${LIBUSB_LIBRARIES}")
message(STATUS "Found libsamplerate: ${LIBSAMPLERATE_LIBRARIES}")
message(STATUS "Found ALSA: ${ALSA_LIBRARIES}")
message(STATUS "Found Readline: ${READLINE_LIBRARIES}")
message(STATUS "Found RtAudio: ${RTAUDIO_LIBRARIES}")
