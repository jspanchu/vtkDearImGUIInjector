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

# Allow the developer to select if Dynamic or Static libraries are built
option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
# For an adobe spectrum look.
option(USE_ADOBE_SPECTRUM_STYLE "Style ImGui with Adobe spectrum look" ON)

# Set the LIB_TYPE variable to STATIC
set(LIB_TYPE STATIC)
if(BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  set(LIB_TYPE SHARED)
endif()

# Create a target for the library
add_library(${PROJECT_NAME} ${LIB_TYPE} "")
generate_export_header(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE "src/vtkDearImGuiInjector.cxx")
if (USE_ADOBE_SPECTRUM_STYLE)
  set(_IMGUI_DIR "adobe_imgui_spectrum")
  target_sources(${PROJECT_NAME} PRIVATE "${_IMGUI_DIR}/imgui_spectrum.cpp")
elseif(IMGUI_DIR) # Custom imgui directory
  set(_IMGUI_DIR IMGUI_DIR)
elseif(IMGUI_DIR)
  set(_IMGUI_DIR "imgui")
endif()

target_sources(${PROJECT_NAME} PRIVATE
  "${_IMGUI_DIR}/imgui.cpp"
  "${_IMGUI_DIR}/imgui_demo.cpp"
  "${_IMGUI_DIR}/imgui_draw.cpp"
  "${_IMGUI_DIR}/imgui_tables.cpp"
  "${_IMGUI_DIR}/imgui_widgets.cpp"
  "${_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

if (EMSCRIPTEN)
  target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_OPENGL_ES3)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_compile_definitions(${PROJECT_NAME} PUBLIC ADOBE_IMGUI_SPECTRUM)
target_include_directories(${PROJECT_NAME} PUBLIC ${_IMGUI_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} ${VTK_LIBRARIES})

# Install targets
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${PROJECT_NAME}.h ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_export.h DESTINATION include)


# -----------------------------------------------------------------------------
# Configure application
# -----------------------------------------------------------------------------

if (NOT EMSCRIPTEN)
  message("Skipping WebAssembly example: This needs to run inside an Emscripten build environment")

  add_executable(test_imgui_vtk "src/test_imgui_vtk.cxx")
  if (USE_ADOBE_SPECTRUM_STYLE)
    target_sources(test_imgui_vtk PRIVATE
      "adobe_imgui_spectrum/imgui_spectrum.cpp"
      "adobe_imgui_spectrum/imgui_spectrum.h"
    )
  endif()

  target_link_libraries(test_imgui_vtk PRIVATE ${PROJECT_NAME} ${VTK_LIBRARIES})
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
target_link_libraries(imgui_vtk_app PRIVATE ${PROJECT_NAME} ${VTK_LIBRARIES})

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

