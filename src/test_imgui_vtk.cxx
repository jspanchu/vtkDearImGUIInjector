/*=========================================================================
  Program:   Visualization Toolkit
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#ifdef __EMSCRIPTEN__
#include "vtkSDL2OpenGLRenderWindow.h"
#include "vtkSDL2RenderWindowInteractor.h"
#endif
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkDearImGUIInjector.h"

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
#ifdef __EMSCRIPTEN__
  vtkNew<vtkSDL2OpenGLRenderWindow> renderWindow;
  vtkNew<vtkSDL2RenderWindowInteractor> renderWindowInteractor;
#else
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
#endif
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  renderWindowInteractor->SetRenderWindow(renderWindow);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  renderWindowInteractor->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  // Create pipeline
  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Add the actors to the scene
  renderer->AddActor(actor);

  // Initialize an overlay with DearImgui elements.
  vtkNew<vtkDearImGUIInjector> dearImGuiOverlay;
  dearImGuiOverlay->Inject(renderWindowInteractor);
  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->SetSize(1920, 1080);
  renderWindow->Render();

  // Start event loop
  renderWindowInteractor->Start();

  return 0;
}
