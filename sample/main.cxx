// SPDX-FileCopyrightText: Copyright 2023 Jaswant Panchumarti
// SPDX-License-Identifier: BSD-3-Clause

#include <sstream>
#include <string>

#include "OverlayUI.h"
#include "vtkDearImGuiInjector.h"

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCameraOrientationRepresentation.h"
#include "vtkCameraOrientationWidget.h"
#include "vtkConeSource.h"
#include "vtkInteractorStyle.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Create a renderer, render window, and interactor
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  vtkNew<vtkRenderWindowInteractor> iren;
  renderWindow->SetMultiSamples(8);
  renderWindow->AddRenderer(renderer);
  iren->SetRenderWindow(renderWindow);

  // Create pipeline
  vtkNew<vtkConeSource> coneSource;
  coneSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(coneSource->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Add the actors to the scene
  renderer->AddActor(actor);

  // Start rendering app
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->Render();

  /// Change to your code begins here. ///
  // Initialize an overlay with DearImgui elements.
  vtkNew<vtkDearImGuiInjector> dearImGuiOverlay;
  dearImGuiOverlay->DebugOn();
  // 💉 the overlay.
  dearImGuiOverlay->Inject(iren);
  // Listens to vtkDearImGuiInjector::ImGuiSetupEvent
  vtkNew<vtkCallbackCommand> uiSetupCmd;
  uiSetupCmd->SetCallback(OverlayUI::setup);
  dearImGuiOverlay->AddObserver(vtkDearImGuiInjector::ImGuiSetupEvent, uiSetupCmd);
  // Listens to vtkDearImGuiInjector::ImGuiDrawEvent
  vtkNew<vtkCallbackCommand> uiDrawCmd;
  uiDrawCmd->SetCallback(OverlayUI::draw);
  dearImGuiOverlay->AddObserver(vtkDearImGuiInjector::ImGuiDrawEvent, uiDrawCmd);
  // You can draw custom user interface elements using ImGui:: namespace.
  /// Change to your code ends here. ///

  vtkNew<vtkCameraOrientationWidget> camManipulator;
  camManipulator->SetParentRenderer(renderer);
  camManipulator->On();
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(camManipulator->GetRepresentation());
  rep->AnchorToLowerRight();

  // Start event loop
  renderWindow->SetSize(1920, 1000);
  iren->EnableRenderOff();
  iren->Start();

  return 0;
}
