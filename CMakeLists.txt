cmake_minimum_required(VERSION 3.14)

set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)

set (CMAKE_CXX_STANDARD 20)

project(Brain-Visualisation)

include_directories("src")

find_package(VTK REQUIRED)

#QT6
find_package(Qt6 REQUIRED COMPONENTS Core Gui OpenGLWidgets Widgets)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
set(QT_MODULES Qt6::Core Qt6::Gui Qt6::OpenGLWidgets Qt6::Widgets)

# Neuron properties preprocessing
add_executable(PreprocessNeuronProperties "src/utility.hpp" "src/preprocessNeuronProperties/preprocessNeuronProperties.cpp" "src/neuronProperties.hpp")

# Network preprocessing
add_executable(PreprocessNetwork "src/utility.hpp" "src/preprocessEdges/preprocessEdges.cpp" "src/edge.hpp")
target_link_libraries(PreprocessNetwork PRIVATE ${VTK_LIBRARIES})

# Brain visualisation
file(GLOB VIS_FILES CONFIGURE_DEPENDS "src/vis/*")
add_executable("${PROJECT_NAME}" ${VIS_FILES} "src/utility.hpp" "src/edge.hpp" "src/neuronProperties.hpp")
target_link_libraries("${PROJECT_NAME}" PRIVATE ${VTK_LIBRARIES} ${QT_MODULES})


vtk_module_autoinit(
  TARGETS "${PROJECT_NAME}"
  MODULES ${VTK_LIBRARIES}
)

