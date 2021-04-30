cmake_minimum_required(VERSION 3.19)
project(hikvision_python)
find_package(SWIG 4.0 COMPONENTS python)
if (SWIG_FOUND)
    message("SWIG found: ${SWIG_EXECUTABLE}")
endif ()
include(${SWIG_USE_FILE})

set(Python_ADDITIONAL_VERSIONS 3.6)
find_package(PythonLibs REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(incCn)
link_directories(lib)

set_property(SOURCE hikvisionpy.i PROPERTY CPLUSPLUS ON)

swig_add_library(
        hikvision_python
        TYPE SHARED
        LANGUAGE python
        SOURCES hikvisionpy.i library.cpp )

target_link_libraries(hikvision_python hcnetsdk PlayCtrl ${OpenCV_LIBS})