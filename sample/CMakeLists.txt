# -----------------------------------------------------------------------------
# Configure application
# -----------------------------------------------------------------------------

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message("Skipping WebAssembly example: This needs to run inside an Emscripten build environment")
  add_executable(main
    "main.cxx"
    "OverlayUI.cxx")
  target_include_directories(main PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include>)
  target_link_libraries(main PRIVATE ${CMAKE_PROJECT_NAME})
  vtk_module_autoinit(
      TARGETS main
      MODULES ${VTK_LIBRARIES})
  # Install targets
  install(TARGETS main
    RUNTIME DESTINATION bin)
  install(FILES "include/Karla-Regular.hpp" "include/OverlayUI.h" DESTINATION include)
  return ()
endif ()


# -----------------------------------------------------------------------------
# WebAssembly build options
# -----------------------------------------------------------------------------
set(DEBUGINFO "NONE" CACHE STRING "Type of debug info")
set_property(CACHE DEBUGINFO PROPERTY
  STRINGS
    NONE              # -g0
    READABLE_JS       # -g1
    PROFILE           # -g2
    DEBUG_NATIVE      # -g3
)
set(OPTIMIZE "BEST" CACHE STRING "Emscripten optimization")
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

# -----------------------------------------------------------------------------
# Compile example code
# -----------------------------------------------------------------------------

add_executable(imgui_vtk_app 
  "main.cxx"
  "OverlayUI.cxx")
target_include_directories(imgui_vtk_app PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_link_libraries(imgui_vtk_app PRIVATE ${CMAKE_PROJECT_NAME})

set(emscripten_link_options)
set(emscripten_compile_options)
list(APPEND emscripten_link_options
  "-lembind"
  "-sMODULARIZE=1"
  "-sEXPORT_NAME=vtkApp"
  "-sENVIRONMENT=web"
  "-sALLOW_MEMORY_GROWTH=1"
  "-sSINGLE_FILE=1"
  "-sMAXIMUM_MEMORY=4GB"
)

set(emscripten_debug_options)
if(DEBUGINFO STREQUAL "NONE")
  list(APPEND emscripten_debug_options
    "-g0"
  )
elseif(DEBUGINFO STREQUAL "READABLE_JS")
  list(APPEND emscripten_debug_options
    "-g1"
  )
  list(APPEND emscripten_link_options
    "-sDEMANGLE_SUPPORT=1"
  )
elseif(DEBUGINFO STREQUAL "PROFILE")
  list(APPEND emscripten_debug_options
    "-g2"
  )
  list(APPEND emscripten_link_options
    "-sDEMANGLE_SUPPORT=1"
  )
elseif(DEBUGINFO STREQUAL "DEBUG_NATIVE")
  list(APPEND emscripten_debug_options
    "-g3"
    "-fsanitize=address"
  )
  list(APPEND emscripten_link_options
    "-sASSERTIONS=1"
    "-sDEMANGLE_SUPPORT=1"
  )
endif()

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------
set(emscripten_optimizations)
if(OPTIMIZE STREQUAL "NO_OPTIMIZATION")
  list(APPEND emscripten_optimizations
    "-O0"
  )
elseif(OPTIMIZE STREQUAL "LITTLE")
  list(APPEND emscripten_optimizations
    "-O1"
  )
elseif(OPTIMIZE STREQUAL "MORE")
  list(APPEND emscripten_optimizations
    "-O2"
  )
elseif(OPTIMIZE STREQUAL "BEST")
  list(APPEND emscripten_optimizations
    "-O3"
  )
elseif(OPTIMIZE STREQUAL "SMALL")
  list(APPEND emscripten_optimizations
    "-Os"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST")
  list(APPEND emscripten_optimizations
    "-Oz"
  )
elseif(OPTIMIZE STREQUAL "SMALLEST_WITH_CLOSURE")
  list(APPEND emscripten_optimizations
    "-Oz"
  )
  list(APPEND emscripten_link_options
    "--closure=1"
  )
endif()

target_compile_options(imgui_vtk_app
  PUBLIC
    ${emscripten_compile_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
)

target_link_options(imgui_vtk_app
  PUBLIC
    ${emscripten_link_options}
    ${emscripten_optimizations}
    ${emscripten_debug_options}
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
      "${PROJECT_SOURCE_DIR}/index.html"
      $<TARGET_FILE_DIR:imgui_vtk_app>
)

