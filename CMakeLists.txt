cmake_minimum_required (VERSION 3.6)
project (vtkDearImGUIInjector)
include (GenerateExportHeader)

find_package(ParaView
  QUIET
)
if (NOT ParaView_FOUND)
  find_package(VTK COMPONENTS 
    CommonColor
    CommonCore
    CommonDataModel
    FiltersCore
    ImagingHybrid
    InteractionStyle
    RenderingContextOpenGL2
    RenderingCore
    RenderingFreeType
    RenderingGL2PSOpenGL2
    RenderingOpenGL2
    QUIET
  )
  if (NOT VTK_FOUND)
    message(FATAL_ERROR "Cannot find VTK installation")
    return ()
  endif()
endif()

message(STATUS "VTK_VERSION: ${VTK_VERSION}")

# Allow the developer to select if Dynamic or Static libraries are built
option(BUILD_SHARED_LIBS "Build Shared Libraries" ON)

# Set the LIB_TYPE variable to STATIC
set(LIB_TYPE STATIC)
if(BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  set(LIB_TYPE SHARED)
endif()

# Create a target for the library
add_library(${PROJECT_NAME} ${LIB_TYPE} "")
generate_export_header(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
"src/vtkDearImGUIInjector.cxx"
"imgui/imgui.cpp"
"imgui/imgui_demo.cpp"
"imgui/imgui_draw.cpp"
"imgui/imgui_tables.cpp"
"imgui/imgui_widgets.cpp"
"imgui/backends/imgui_impl_opengl3.cpp"
)
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM="vtk_glew.h")
target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_include_directories(${PROJECT_NAME} PRIVATE "imgui" "${PROJECT_BINARY_DIR}")
target_link_libraries(${PROJECT_NAME} ${VTK_LIBRARIES})

# Install targets
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(FILES ${PROJECT_NAME}.h ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_export.h DESTINATION include)

add_executable(test_imgui_vtk "src/test_imgui_vtk.cxx")
target_include_directories(test_imgui_vtk PRIVATE "${PROJECT_BINARY_DIR}")
target_link_libraries(test_imgui_vtk PRIVATE ${PROJECT_NAME} ${VTK_LIBRARIES})
vtk_module_autoinit(
    TARGETS test_imgui_vtk
    MODULES ${VTK_LIBRARIES}
)
