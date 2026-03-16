# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)


if("$ENV{IDF_PATH}" STREQUAL "")
	file(GLOB _idf_candidates "$ENV{HOME}/.espressif/v*/esp-idf")
	if(_idf_candidates)
		list(SORT _idf_candidates ORDER DESCENDING)
		list(GET _idf_candidates 0 _idf_path)
		if(EXISTS "${_idf_path}/tools/cmake/project.cmake")
			set(ENV{IDF_PATH} "${_idf_path}")
		endif()
	endif()
endif()

include("$ENV{IDF_PATH}/tools/cmake/project.cmake")
set(PROJECT_VERSION "3.0")
project(shrimpiot)

idf_build_set_property(COMPILE_OPTIONS "-Wno-missing-field-initializers -Wno-reorder" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-w" APPEND)
