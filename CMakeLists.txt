cmake_minimum_required(VERSION 3.0)
project(cranks-renderer C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES /EHsc /ZI")

# GLFW
set(GLFW_DIR third_party/glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

# Dear ImGui
set(IMGUI_DIR third_party/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

# xc imgui source
file(GLOB IMGUI_CODE
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
)

# Libraries
find_package(Vulkan REQUIRED)
#find_library(VULKAN_LIBRARY
  #NAMES vulkan vulkan-1)
#set(LIBRARIES "glfw;${VULKAN_LIBRARY}")
set(LIBRARIES "glfw;Vulkan::Vulkan")

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

# file(GLOB sources src/*.cpp)

# cranks-renderer code
file(GLOB PBR_CODE
  src/main.cpp
  src/renderer_ui.h
  src/renderer_ui.cpp
  src/gui_main.h
  src/gui_main.cpp
  src/vulkan/*.h
  src/vulkan/*.cpp
  src/core/*.h
  src/core/*.cpp
  src/shape/*.h
  src/shape/*.cpp
  src/tool/*.h
  src/tool/*.cpp
  src/light/*.h
  src/light/*.cpp
  src/integrator/*.h
  src/integrator/*.cpp
  src/base/*.h
  src/base/*.cpp
)

# SOURCE_GROUP(PBR_CODE FILES ${PBR_CODE_FILES})
source_group(cranks-renderer src)
source_group(base src/base)
source_group(core src/core)
source_group(shape src/shape)
source_group(tool src/tool)
source_group(light src/light)
source_group(integrator src/integrator)
source_group(vulkan src/vulkan)

source_group(imgui ${IMGUI_DIR})

#source_group(TREE "src" FILES ${PBR_CODE})
#source_group(cranks-renderer  FILES ${PBR_CODE})

add_executable(cranks-renderer ${PBR_CODE} ${IMGUI_CODE})

target_link_libraries(cranks-renderer ${LIBRARIES})
target_compile_definitions(cranks-renderer PUBLIC -DImTextureID=ImU64)

# c++ 20
target_compile_features(cranks-renderer PRIVATE cxx_std_20)

# linker generate debug info
target_link_options(cranks-renderer PUBLIC /DEBUG)
