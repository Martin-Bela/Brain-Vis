cmake_minimum_required(VERSION 3.14)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

set (CMAKE_CXX_STANDARD 20)

project(Brain-Visualisation)

set(SOURCES "src/main.cpp")
add_executable("${PROJECT_NAME}" ${SOURCES})

find_package(VTK REQUIRED)

target_link_libraries("${PROJECT_NAME}" PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS "${PROJECT_NAME}"
  MODULES ${VTK_LIBRARIES}
)