cmake_minimum_required(VERSION 3.14)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

set (CMAKE_CXX_STANDARD 20)

project(Brain-Visualisation)

include_directories("src")

add_executable(PreprocessNeuronProperties "src/preprocessNeuronProperties/preprocessNeuronProperties.cpp" "src/neuronProperties.hpp")

add_executable("${PROJECT_NAME}" "src/vis/main.cpp" "src/vis/context.hpp" "src/vis/utility.hpp" "src/vis/utility.cpp" "src/vis/slider.hpp"
	"src/neuronProperties.hpp")

find_package(VTK REQUIRED)

target_link_libraries("${PROJECT_NAME}" PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS "${PROJECT_NAME}"
  MODULES ${VTK_LIBRARIES}
)