# SPDX-FileCopyrightText: Copyright 2023 Jaswant Panchumarti
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required (VERSION 3.6)
project (vtkDearImGuiInjector)
include (GenerateExportHeader)

find_package(VTK
  COMPONENTS
    FiltersSources
    InteractionStyle
    InteractionWidgets
    RenderingOpenGL2
    RenderingUI 
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

message(STATUS "VTK_VERSION: ${VTK_VERSION}")

# For an adobe spectrum look.
option(USE_ADOBE_SPECTRUM_STYLE "Style ImGui with Adobe spectrum look" ON)

add_subdirectory(src)
add_subdirectory(sample)
