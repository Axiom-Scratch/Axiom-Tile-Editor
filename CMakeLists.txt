cmake_minimum_required(VERSION 3.20)

project(tile_editor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Options.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Warnings.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_SYSTEM_GLFW "Use system-installed GLFW" OFF)

if (USE_SYSTEM_GLFW)
  find_package(glfw3 CONFIG QUIET)
  if (TARGET glfw)
    set(GLFW_TARGET glfw)
  elseif (TARGET glfw3::glfw)
    set(GLFW_TARGET glfw3::glfw)
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
    set(GLFW_TARGET PkgConfig::GLFW3)
  endif()
else()
  include(FetchContent)

  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
  )
  FetchContent_MakeAvailable(glfw)
  set(GLFW_TARGET glfw)
endif()

find_package(OpenGL REQUIRED)

add_library(glad STATIC
  external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
  external/glad/include
)

add_library(stb_image STATIC
  external/stb/stb_image.cpp
)

target_include_directories(stb_image PUBLIC
  external/stb
)

file(GLOB_RECURSE TILE_EDITOR_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_executable(tile_editor
  ${TILE_EDITOR_SOURCES}
)

target_include_directories(tile_editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/external
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

target_link_libraries(tile_editor PRIVATE
  ${GLFW_TARGET}
  glad
  OpenGL::GL
  stb_image
)

target_compile_definitions(tile_editor PRIVATE
  $<$<CONFIG:Debug>:TE_DEBUG>
)

if (TE_ENABLE_GL_DEBUG)
  target_compile_definitions(tile_editor PRIVATE TE_ENABLE_GL_DEBUG)
endif()

te_set_warnings(tile_editor)

set_target_properties(tile_editor PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
