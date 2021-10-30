cmake_minimum_required (VERSION 3.6)
project (vtkDearImGuiInjector)
include (GenerateExportHeader)

find_package(ParaView
  QUIET
)
if (NOT ParaView_FOUND)
  find_package(VTK)
  if (NOT VTK_FOUND)
    message(FATAL_ERROR "Cannot find VTK installation")
    return ()
  endif()
endif()

message(STATUS "VTK_VERSION: ${VTK_VERSION}")

# For an adobe spectrum look.
option(USE_ADOBE_SPECTRUM_STYLE "Style ImGui with Adobe spectrum look" ON)

if (USE_ADOBE_SPECTRUM_STYLE)
  set(_IMGUI_DIR "adobe_imgui_spectrum")
  list(APPEND _proj_definitions ADOBE_IMGUI_SPECTRUM)
elseif(IMGUI_DIR) # From cli -DIMGUI_DIR
  set(_IMGUI_DIR IMGUI_DIR)
else()
  set(_IMGUI_DIR "imgui")
endif()

# sources and headers
file(GLOB IMGUI_SOURCES ${_IMGUI_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${_IMGUI_DIR}/*.h)
list(APPEND _proj_headers
  ${IMGUI_HEADERS}
  "${_IMGUI_DIR}/backends/imgui_impl_opengl3.h"
  "include/vtkDearImGuiInjector.h"
)
list(APPEND _proj_sources
  ${IMGUI_SOURCES}
  "${_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
  "src/vtkDearImGuiInjector.cxx"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  list(APPEND _proj_definitions IMGUI_IMPL_OPENGL_ES3)
endif()

# library target
add_library(${CMAKE_PROJECT_NAME} STATIC ${_proj_sources})
target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC ${_proj_definitions})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${_IMGUI_DIR}>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${_IMGUI_DIR}/backends>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${VTK_LIBRARIES})
generate_export_header(${CMAKE_PROJECT_NAME})

# Install targets
install(TARGETS ${CMAKE_PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_PROJECT_NAME}.h ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}_export.h DESTINATION include)
install(FILES ${_proj_headers} DESTINATION include)

# -----------------------------------------------------------------------------
# Configure application
# -----------------------------------------------------------------------------

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message("Skipping WebAssembly example: This needs to run inside an Emscripten build environment")

  add_executable(test_imgui_vtk "src/test_imgui_vtk.cxx")

  target_link_libraries(test_imgui_vtk PRIVATE ${CMAKE_PROJECT_NAME})
  vtk_module_autoinit(
      TARGETS test_imgui_vtk
      MODULES ${VTK_LIBRARIES}
  )
  return ()
endif ()


# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------

set(emscripten_options)
list(APPEND emscripten_options
  "--bind"
  "-g3"
  "SHELL:-s EXPORT_NAME=vtkApp"
  "SHELL:-s ALLOW_MEMORY_GROWTH=1"
  "SHELL:-s DEMANGLE_SUPPORT=1"
  "SHELL:-s EMULATE_FUNCTION_POINTER_CASTS=0"
  "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=0"
  "SHELL:-s MODULARIZE=1"
  "SHELL:-s USE_PTHREADS=0"
  "SHELL:-s WASM=1"
)

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------

set(OPTIMIZE "SMALLEST_WITH_CLOSURE" CACHE STRING "Emscripten optimization")
set_property(CACHE OPTIMIZE PROPERTY
  STRINGS
    NO_OPTIMIZATION       # -O0
    LITTLE                # -O1
    MORE                  # -O2
    BEST                  # -O3
    SMALL                 # -Os
    SMALLEST              # -Oz
    SMALLEST_WITH_CLOSURE # -Oz --closure 1
)

if(OPTIMIZE STREQUAL "NO_OPTIMIZATION")
  # imgui_vtk.js    659K
  # imgui_vtk.wasm  4.9M
  # time => 4 minutes 3 seconds
  list(APPEND emscripten_options
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "LITTLE")
  # imgui_vtk.js    529K
  # imgui_vtk.wasm  5.9M
  list(APPEND emscripten_options
    "-O1"
  )
elseif(OPTIMIZE STREQUAL "MORE")
  # imgui_vtk.js    529K
  # imgui_vtk.wasm  5.3M
  list(APPEND emscripten_options
    "-O2"
  )
elseif(OPTIMIZE STREQUAL "BEST")
  # imgui_vtk.js    529K
  # imgui_vtk.wasm  4.9M
  # time => 4 minutes 7 seconds
  list(APPEND emscripten_options
    "-O3"
  )
elseif(OPTIMIZE STREQUAL "SMALL")
  # imgui_vtk.js    529K
  # imgui_vtk.wasm  4.9M
  list(APPEND emscripten_options
    "-Os"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST")
  # imgui_vtk.js    659K
  # imgui_vtk.wasm  4.9M
  list(APPEND emscripten_options
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST_WITH_CLOSURE")
  # imgui_vtk.js    659K
  # imgui_vtk.wasm  4.9M
  list(APPEND emscripten_options
    "-Oz"
    "SHELL:--closure 1"
  )
endif()

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(imgui_vtk_app "src/test_imgui_vtk.cxx")
target_link_libraries(imgui_vtk_app PRIVATE ${CMAKE_PROJECT_NAME} ${VTK_LIBRARIES})

target_link_libraries(imgui_vtk_app
  PRIVATE
    VTK::CommonColor
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersCore
    VTK::FiltersSources
    VTK::InteractionStyle
    VTK::InteractionWidgets
    VTK::RenderingCore
    VTK::RenderingFreeType
    VTK::RenderingOpenGL2
)

target_compile_options(imgui_vtk_app
  PUBLIC
    ${emscripten_options}
)

target_link_options(imgui_vtk_app
  PUBLIC
    ${emscripten_options}
)

# -----------------------------------------------------------------------------
# VTK modules initialization
# -----------------------------------------------------------------------------

vtk_module_autoinit(
  TARGETS  imgui_vtk_app
  MODULES  ${VTK_LIBRARIES}
)

# -----------------------------------------------------------------------------
# Copy HTML to build directory
# -----------------------------------------------------------------------------

add_custom_command(
  TARGET imgui_vtk_app
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:imgui_vtk_app>
)

