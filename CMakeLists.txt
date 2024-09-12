
cmake_minimum_required(VERSION 3.25)
project(Bare-Annoy-Addon LANGUAGES CXX)

# Include the Bare runtime
include(bare)
add_bare_module(Bare-Annoy-Addon)

# Include directories for Annoy headers (adjust the path based on where your headers are)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link the Annoy headers to your Bare addon
target_sources(${Bare-Annoy-Addon} PRIVATE binding.cpp)