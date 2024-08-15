cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
  prism
  VERSION 0.1.0
  DESCRIPTION "A Vulkan rendered C++ application framework using GLFW and ImGui."
  LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(prism_prism
  # Prism
  src/prism.cpp
  src/window.cpp
  src/renderer.cpp
)
add_library(prism::prism ALIAS prism_prism)

include(GenerateExportHeader)
generate_export_header(
  prism_prism
  BASE_NAME prism
  EXPORT_FILE_NAME export/prism/prism_export.hpp
  CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(prism_prism PUBLIC PRISM_STATIC_DEFINE)
endif()

set_target_properties(
  prism_prism PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
  EXPORT_NAME prism
  OUTPUT_NAME prism
)

target_include_directories(
  prism_prism ${warning_guard}
  PUBLIC
  "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
  prism_prism SYSTEM
  PUBLIC
  "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(prism_prism PUBLIC cxx_std_20)

# Link dependencies
find_package(fmt REQUIRED)
target_link_libraries(prism_prism PRIVATE fmt::fmt)

find_package(glfw3 REQUIRED)
target_link_libraries(prism_prism PRIVATE glfw)

find_package(Freetype REQUIRED)
target_link_libraries(prism_prism PRIVATE Freetype::Freetype)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(prism_prism PRIVATE imgui::imgui)

find_package(Vulkan REQUIRED)
target_include_directories(prism_prism PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(prism_prism PRIVATE ${Vulkan_LIBRARIES})

# Prevent windows.h from including winsock.h (for kmbox code)
add_definitions(-D_WINSOCKAPI_)

# Link libraries
# target_link_libraries(
#   prism_prism
#   PRIVATE
#   "${PROJECT_SOURCE_DIR}/lib/lib.lib"
# )

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT prism_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
    AUTHOR_WARNING
    "Developer mode is intended for developers of prism"
  )
endif()

include(cmake/dev-mode.cmake)
