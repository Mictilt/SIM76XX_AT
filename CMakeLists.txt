cmake_minimum_required(VERSION 3.5)

# Set the project name
set(PROJECT_NAME "modem_simcomA7600")

# Include ESP-IDF project cmake file
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Create project
project(${PROJECT_NAME})

# Add required components to the build

# Set compiler options if needed
target_compile_options(${PROJECT_NAME}.elf PRIVATE -Wno-error=format)