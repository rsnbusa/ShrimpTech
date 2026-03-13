# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VERSION "3.0")
project(shrimpiot)

idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers -Wno-reorder" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-w" APPEND)
