cmake_minimum_required(VERSION 3.20)

project(tile_editor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Options.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Warnings.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (DEFINED ENV{FETCHCONTENT_BASE_DIR})
  set(FETCHCONTENT_BASE_DIR "$ENV{FETCHCONTENT_BASE_DIR}" CACHE PATH "" FORCE)
endif()

option(USE_SUBMODULE_DEPS "Use deps from third_party/ submodules" ON)
option(USE_SYSTEM_GLFW "Use system installed GLFW" OFF)
option(IMGUI_DOCKING "Use ImGui docking branch" ON)

if (IMGUI_DOCKING)
  set(IMGUI_GIT_TAG docking)
else()
  set(IMGUI_GIT_TAG v1.90.9)
endif()

include(FetchContent)

set(GLFW_TARGET "")
set(GLFW_SUBMODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party/glfw)

if (USE_SUBMODULE_DEPS AND EXISTS ${GLFW_SUBMODULE_DIR}/CMakeLists.txt)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

  add_subdirectory(third_party/glfw)
  set(GLFW_TARGET glfw)
elseif (USE_SUBMODULE_DEPS)
  message(WARNING "USE_SUBMODULE_DEPS=ON but third_party/glfw not found. Falling back to system/FetchContent.")
endif()

if (NOT GLFW_TARGET)
  if (USE_SYSTEM_GLFW)
    find_package(glfw3 CONFIG QUIET)
    if (TARGET glfw3::glfw)
      set(GLFW_TARGET glfw3::glfw)
    elseif (TARGET glfw)
      set(GLFW_TARGET glfw)
    else()
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
      set(GLFW_TARGET PkgConfig::GLFW3)
    endif()
  else()
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
      glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.3.9
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw)
    set(GLFW_TARGET glfw)
  endif()
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG ${IMGUI_GIT_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

find_package(OpenGL REQUIRED)

add_library(glad STATIC
  external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
  external/glad/include
)
target_compile_options(glad PRIVATE ${TE_WARNINGS_DEPS})
target_compile_options(glad PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-pedantic>)

add_library(stb_image STATIC
  external/stb/stb_image.cpp
)

target_include_directories(stb_image PUBLIC
  external/stb
)
target_compile_options(stb_image PRIVATE ${TE_WARNINGS_DEPS})

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC
  ${IMGUI_SOURCES}
)

target_include_directories(imgui SYSTEM PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_compile_options(imgui PRIVATE ${TE_WARNINGS_DEPS})

target_compile_definitions(imgui PUBLIC
  IMGUI_IMPL_OPENGL_LOADER_GLAD
)

target_link_libraries(imgui PUBLIC
  ${GLFW_TARGET}
  OpenGL::GL
)

file(GLOB_RECURSE TILE_EDITOR_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

add_executable(tile_editor
  ${TILE_EDITOR_SOURCES}
)

target_include_directories(tile_editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(tile_editor SYSTEM PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/external
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

if (DEFINED glfw_SOURCE_DIR)
  target_include_directories(tile_editor SYSTEM PRIVATE ${glfw_SOURCE_DIR}/include)
endif()

if (EXISTS ${GLFW_SUBMODULE_DIR}/include)
  target_include_directories(tile_editor SYSTEM PRIVATE ${GLFW_SUBMODULE_DIR}/include)
endif()

target_link_libraries(tile_editor PRIVATE
  ${GLFW_TARGET}
  glad
  OpenGL::GL
  stb_image
  imgui
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
