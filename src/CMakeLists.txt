if (USE_ADOBE_SPECTRUM_STYLE)
  set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/adobe_imgui_spectrum")
  list(APPEND lib_definitions ADOBE_IMGUI_SPECTRUM)
elseif(IMGUI_DIR) # From cli -DIMGUI_DIR
  set(IMGUI_DIR IMGUI_DIR)
else()
  set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/imgui")
endif()

# sources and headers
file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${IMGUI_DIR}/*.h)
list(APPEND lib_headers
  ${IMGUI_HEADERS}
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.h"
  "vtkDearImGuiInjector.h"
)
list(APPEND lib_sources
  ${IMGUI_SOURCES}
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
  "vtkDearImGuiInjector.cxx"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  list(APPEND lib_definitions IMGUI_IMPL_OPENGL_ES3)
endif()

# library target
add_library(vtkDearImGuiInjector STATIC ${lib_sources})
target_compile_definitions(vtkDearImGuiInjector PUBLIC ${lib_definitions})
target_include_directories(vtkDearImGuiInjector PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<BUILD_INTERFACE:${IMGUI_DIR}>
  $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(vtkDearImGuiInjector PUBLIC ${VTK_LIBRARIES})
generate_export_header(vtkDearImGuiInjector)

# Install targets
install(TARGETS vtkDearImGuiInjector
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES vtkDearImGuiInjector.h ${CMAKE_CURRENT_BINARY_DIR}/vtkDearImGuiInjector_export.h DESTINATION include)
install(FILES ${lib_headers} DESTINATION include)
