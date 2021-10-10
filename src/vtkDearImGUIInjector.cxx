#include <cstring>
#include <iostream>
#include <vtkCommand.h>
#include <vtkDearImGUIInjector.h>

#include <chrono>
#include <string>
#include <unordered_map>

#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#endif

vtkStandardNewMacro(vtkDearImGUIInjector);

namespace
{
std::unordered_map<std::string, int> keySymToCode(
  { { "Tab", 0 }, { "LeftArrow", 1 }, { "RightArrow", 2 }, { "UpArrow", 3 }, { "DownArrow", 4 },
    { "PageUp", 5 }, { "PageDown", 6 }, { "Home", 7 }, { "End", 8 }, { "Insert", 9 },
    { "Delete", 10 }, { "BackSpace", 11 }, { "Space", 12 }, { "Enter", 13 }, { "Escape", 14 },
    { "KeyPadEnter", 15 }, { "A", 16 }, { "C", 17 }, { "V", 18 }, { "X", 19 }, { "Y", 20 },
    { "Z", 21 }, { "a", 16 }, { "c", 17 }, { "v", 18 }, { "x", 19 }, { "y", 20 }, { "z", 21 } });

const std::unordered_map<int, int> imguiToVtkCursors(
  { { ImGuiMouseCursor_None, VTK_CURSOR_DEFAULT }, { ImGuiMouseCursor_Arrow, VTK_CURSOR_ARROW },
    { ImGuiMouseCursor_TextInput, VTK_CURSOR_DEFAULT },
    { ImGuiMouseCursor_ResizeAll, VTK_CURSOR_SIZEALL },
    { ImGuiMouseCursor_ResizeNS, VTK_CURSOR_SIZENS },
    { ImGuiMouseCursor_ResizeEW, VTK_CURSOR_SIZEWE },
    { ImGuiMouseCursor_ResizeNESW, VTK_CURSOR_SIZENE },
    { ImGuiMouseCursor_ResizeNWSE, VTK_CURSOR_SIZENW }, { ImGuiMouseCursor_Hand, VTK_CURSOR_HAND },
    { ImGuiMouseCursor_NotAllowed, VTK_CURSOR_DEFAULT } });
}

const unsigned long vtkDearImGUIInjector::ImGUIDrawEvent = vtkCommand::UserEvent + 1;

vtkDearImGUIInjector::vtkDearImGUIInjector()
{
  // Start DearImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
}

vtkDearImGUIInjector::~vtkDearImGUIInjector()
{
  // Destroy DearImGUi
  if (ImGui::GetCurrentContext())
  {
    ImGui::DestroyContext();
  }
}

void vtkDearImGUIInjector::Inject(vtkRenderWindowInteractor* interactor)
{
  if (this->FinishedSetup)
  {
    vtkErrorMacro(<< "Inject must be called only once!");
  }

  this->Interactor = interactor;
  auto renWin = interactor->GetRenderWindow();
  if (!renWin)
  {
    return;
  }

  // intercept interactor
  this->EventCallbackCommand->SetClientData(this);
  this->EventCallbackCommand->SetCallback(&vtkDearImGUIInjector::DispatchEv);
  interactor->AddObserver(vtkCommand::StartEvent, this, &vtkDearImGUIInjector::PumpEv);

  // intercept renderer
  renWin->AddObserver(vtkCommand::StartEvent, this, &vtkDearImGUIInjector::BeginDearImGUIOverlay);
  renWin->AddObserver(vtkCommand::RenderEvent, this, &vtkDearImGUIInjector::RenderDearImGUIOverlay);

  // Safely exit when vtk app exits
  interactor->AddObserver(vtkCommand::ExitEvent, this, &vtkDearImGUIInjector::TearDown);
}

bool vtkDearImGUIInjector::SetUp(vtkRenderWindow* renWin)
{
  if (renWin->GetNeverRendered())
  {
    vtkDebugMacro(<< "Init called, but render library is not setup. Hold on..");
    return false; // too early
  }
  if (this->FinishedSetup)
  {
    return this->FinishedSetup;
  }

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values
                                                        // (optional)

  // Keyboard mapping. Dear ImGui will use those indices to peek into the
  // io.KeysDown[] array.
  io.KeyMap[ImGuiKey_Tab] = keySymToCode["Tab"];
  io.KeyMap[ImGuiKey_LeftArrow] = keySymToCode["LeftArrow"];
  io.KeyMap[ImGuiKey_RightArrow] = keySymToCode["RightArrow"];
  io.KeyMap[ImGuiKey_UpArrow] = keySymToCode["UpArrow"];
  io.KeyMap[ImGuiKey_DownArrow] = keySymToCode["DownArrow"];
  io.KeyMap[ImGuiKey_PageUp] = keySymToCode["PageUp"];
  io.KeyMap[ImGuiKey_PageDown] = keySymToCode["PageDown"];
  io.KeyMap[ImGuiKey_Home] = keySymToCode["Home"];
  io.KeyMap[ImGuiKey_End] = keySymToCode["End"];
  io.KeyMap[ImGuiKey_Insert] = keySymToCode["Insert"];
  io.KeyMap[ImGuiKey_Delete] = keySymToCode["Delete"];
  io.KeyMap[ImGuiKey_Backspace] = keySymToCode["BackSpace"];
  io.KeyMap[ImGuiKey_Space] = keySymToCode["Space"];
  io.KeyMap[ImGuiKey_Enter] = keySymToCode["Enter"];
  io.KeyMap[ImGuiKey_Escape] = keySymToCode["Escape"];
  io.KeyMap[ImGuiKey_KeyPadEnter] = keySymToCode["KeyPadEnter"];
  io.KeyMap[ImGuiKey_A] = keySymToCode["a"];
  io.KeyMap[ImGuiKey_C] = keySymToCode["c"];
  io.KeyMap[ImGuiKey_V] = keySymToCode["v"];
  io.KeyMap[ImGuiKey_X] = keySymToCode["x"];
  io.KeyMap[ImGuiKey_Y] = keySymToCode["y"];
  io.KeyMap[ImGuiKey_Z] = keySymToCode["z"];
#if defined(_WIN32)
  io.BackendPlatformName = renWin->GetClassName();
  io.ImeWindowHandle = renWin->GetGenericWindowId();
#endif
  this->FinishedSetup = true;
  return ImGui_ImplOpenGL3_Init();
}

void vtkDearImGUIInjector::TearDown(vtkObject* caller, unsigned long eid, void* callData)
{
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  if (interactor != nullptr)
  {
    interactor->SetDone(true);
  }
  ImGui_ImplOpenGL3_Shutdown();
}

void vtkDearImGUIInjector::BeginDearImGUIOverlay(
  vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "BeginDearImGUIOverlay");
  auto renWin = vtkRenderWindow::SafeDownCast(caller);

  // Ensure valid DearImGui context exists.
  if (!ImGui::GetCurrentContext())
  {
    vtkDebugMacro(<< "BeginDearImGUIOverlay called, but DearImGui context does not exist.");
    return;
  }

  // Ensure DearImGui has been configured to render with OpenGL
  if (!this->SetUp(renWin))
  {
    return; // try next time when vtkRenderWindow finished first render
  }

  // Setup display size (every frame to accommodate for window resizing)
  ImGuiIO& io = ImGui::GetIO();
  int* size = renWin->GetSize();
  const int& w = size[0];
  const int& h = size[1];
  io.DisplaySize = ImVec2((float)w, (float)h);
  io.DisplayFramebufferScale = ImVec2(1, 1);

  // Increment time for DearImGUI
  using namespace std::chrono;
  auto currentTime = float(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()) / 1000.f;
  io.DeltaTime =  this->Time > 0.0 ? (currentTime - this->Time) : (1.0f / 60.0f);
  this->Time = currentTime;

  auto interactor = renWin->GetInteractor();
  this->UpdateMousePosAndButtons(interactor);
  this->UpdateMouseCursor(renWin);

  // Begin ImGUI drawing
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
  // Menu Bar
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Input"))
    {
      ImGui::MenuItem("Grab Mouse", NULL, &this->GrabMouse);
      ImGui::MenuItem("Grab Keyboard", NULL, &this->GrabKeyboard);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tools"))
    {
      ImGui::MenuItem("ImGui Demo", NULL, &this->ShowDemo);
      ImGui::MenuItem("Metrics/Debugger", NULL, &this->ShowAppMetrics);
      ImGui::MenuItem("Style Editor", NULL, &this->ShowAppStyleEditor);
      ImGui::MenuItem("About Dear ImGui", NULL, &this->ShowAppAbout);
      ImGui::MenuItem("Grab Mouse", NULL, &this->GrabMouse);
      ImGui::MenuItem("Grab Keyboard", NULL, &this->GrabKeyboard);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  if (this->ShowDemo)
  {
    ImGui::ShowDemoWindow(&this->ShowDemo);
  }
  if (this->ShowAppMetrics)
  {
    ImGui::ShowMetricsWindow(&this->ShowAppMetrics);
  }
  if (this->ShowAppStyleEditor)
  {
    ImGui::Begin("Style editor", &this->ShowAppStyleEditor);
    ImGui::ShowStyleEditor();
    ImGui::End();
  }
  if (this->ShowAppAbout)
  {
    ImGui::ShowAboutWindow(&this->ShowAppAbout);
  }
  this->InvokeEvent(ImGUIDrawEvent);
}

void vtkDearImGUIInjector::RenderDearImGUIOverlay(
  vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "RenderDearImGUIOverlay");
  auto renWin = vtkRenderWindow::SafeDownCast(caller);
  auto openGLrenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  ImGuiIO& io = ImGui::GetIO();
  if (io.Fonts->IsBuilt())
  {
    ImGui::Render();
    auto fbo = openGLrenWin->GetRenderFramebuffer();
    fbo->Bind();
    fbo->ActivateDrawBuffer(
      (openGLrenWin->GetStereoRender() && openGLrenWin->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
        ? 1
        : 0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    fbo->UnBind();
  }
}

void vtkDearImGUIInjector::InstallEventCallback(vtkRenderWindowInteractor* interactor)
{
  auto iObserver = interactor->GetInteractorStyle();
  if (iObserver == nullptr)
  {
    return;
  }
  auto styleBase = vtkInteractorStyle::SafeDownCast(iObserver);
  if (styleBase == nullptr)
  {
    return;
  }

  this->currentIStyle = nullptr;
  if (styleBase->IsA("vtkInteractorStyleSwitchBase"))
  {
    this->currentIStyle = vtkInteractorStyleSwitch::SafeDownCast(styleBase)->GetCurrentStyle();
  }
  else
  {
    this->currentIStyle = styleBase;
  }

  this->currentIStyle->AddObserver(vtkCommand::EnterEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::LeaveEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MouseMoveEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(
    vtkCommand::LeftButtonDoubleClickEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(
    vtkCommand::MiddleButtonDoubleClickEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::RightButtonPressEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(
    vtkCommand::RightButtonDoubleClickEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MouseWheelForwardEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MouseWheelBackwardEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MouseWheelLeftEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::MouseWheelRightEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::ExposeEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::ConfigureEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::TimerEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::KeyPressEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::KeyReleaseEvent, this->EventCallbackCommand, 1.0);

  this->currentIStyle->AddObserver(vtkCommand::CharEvent, this->EventCallbackCommand, 1.0);
}

void vtkDearImGUIInjector::UninstallEventCallback()
{
  this->currentIStyle->RemoveObserver(this->EventCallbackCommand);
}

void vtkDearImGUIInjector::UpdateMousePosAndButtons(vtkRenderWindowInteractor* interactor)
{
  // Update buttons
  ImGuiIO& io = ImGui::GetIO();
  for (int i = 0; i < 3; i++)
  {
    io.MouseDown[i] = this->MouseJustPressed[i];
  }

  // Update mouse position
#ifdef __EMSCRIPTEN__
  this->Focused = true; // Emscripten
#endif
  if (this->Focused)
  {
    int mouse_x, mouse_y;
    interactor->GetLastEventPosition(mouse_x, mouse_y);
    io.MousePos =
      ImVec2(static_cast<float>(mouse_x), io.DisplaySize.y - static_cast<float>(mouse_y));
  }
}

void vtkDearImGUIInjector::UpdateMouseCursor(vtkRenderWindow* renWin)
{
  ImGuiIO& io = ImGui::GetIO();
  if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
    return;

  ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
  if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
  {
    // Hide OS mouse cursor if DearImgui is drawing it or if it wants no cursor
    renWin->HideCursor();
  }
  else
  {
    // Show OS mouse cursor
    renWin->SetCurrentCursor(imguiToVtkCursors.at(imgui_cursor));
    renWin->ShowCursor();
  }
}
#ifdef __EMSCRIPTEN__
namespace
{

void mainLoopCallback(void* arg)
{
  vtkDearImGUIInjector* self = static_cast<vtkDearImGUIInjector*>(arg);
  vtkRenderWindowInteractor* interactor = self->Interactor;
  vtkRenderWindow* renWin = interactor->GetRenderWindow();

  self->InstallEventCallback(interactor);
  interactor->ProcessEvents();
  self->UninstallEventCallback();
  renWin->Render();
}
}
#endif

void vtkDearImGUIInjector::PumpEv(vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "PumpEv");
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto renWin = interactor->GetRenderWindow();
  interactor->Enable();
  interactor->Initialize();

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(&mainLoopCallback, (void*)this, 0, 1);
#else
  while (!interactor->GetDone())
  {
    this->InstallEventCallback(interactor);
    interactor->ProcessEvents();
    this->UninstallEventCallback();
    renWin->Render();
  }
#endif
}

void vtkDearImGUIInjector::DispatchEv(
  vtkObject* caller, unsigned long eid, void* clientData, void* callData)
{
  // auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto iStyle = vtkInteractorStyle::SafeDownCast(caller);
  auto self = reinterpret_cast<vtkDearImGUIInjector*>(clientData);

  std::string keySym = "";
  int keyCode = -1;

  ImGuiIO& io = ImGui::GetIO();
  (void)io;

  switch (eid)
  {
    case vtkCommand::EnterEvent:
    {
      self->Focused = true;
      break;
    }
    case vtkCommand::LeaveEvent:
    {
      self->Focused = false;

      break;
    }
    case vtkCommand::MouseMoveEvent:
    {
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMouseMove();
      }
      break;
    }
    case vtkCommand::LeftButtonPressEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Left] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnLeftButtonDown();
      }
      break;
    }
    case vtkCommand::LeftButtonReleaseEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Left] = false;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnLeftButtonUp();
      }
      break;
    }
    case vtkCommand::LeftButtonDoubleClickEvent:
    {
      io.MouseDoubleClicked[ImGuiMouseButton_Left] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnLeftButtonDoubleClick();
      }
      break;
    }
    case vtkCommand::MiddleButtonPressEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Middle] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMiddleButtonDown();
      }
      break;
    }
    case vtkCommand::MiddleButtonReleaseEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Middle] = false;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMiddleButtonUp();
      }
      break;
    }
    case vtkCommand::MiddleButtonDoubleClickEvent:
    {
      io.MouseDoubleClicked[ImGuiMouseButton_Middle] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMiddleButtonDoubleClick();
      }
      break;
    }
    case vtkCommand::RightButtonPressEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Right] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnRightButtonDown();
      }
      break;
    }
    case vtkCommand::RightButtonReleaseEvent:
    {
      self->MouseJustPressed[ImGuiMouseButton_Right] = false;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnRightButtonUp();
      }
      break;
    }
    case vtkCommand::RightButtonDoubleClickEvent:
    {
      io.MouseDoubleClicked[ImGuiMouseButton_Right] = true;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnRightButtonDoubleClick();
      }
      break;
    }
    case vtkCommand::MouseWheelBackwardEvent:
    {
      io.MouseWheel = -1;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMouseWheelBackward();
      }
      break;
    }
    case vtkCommand::MouseWheelForwardEvent:
    {
      io.MouseWheel = 1;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMouseWheelForward();
      }
      break;
    }
    case vtkCommand::MouseWheelLeftEvent:
    {
      io.MouseWheelH = 1;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMouseWheelLeft();
      }
      break;
    }
    case vtkCommand::MouseWheelRightEvent:
    {
      io.MouseWheelH = -1;
      if (!io.WantCaptureMouse || (io.WantCaptureMouse && self->GrabMouse))
      {
        iStyle->OnMouseWheelRight();
      }
      break;
    }
    case vtkCommand::ExposeEvent:
    {
      iStyle->OnExpose();
      break;
    }
    case vtkCommand::ConfigureEvent:
    {
      iStyle->OnConfigure();
      break;
    }
    case vtkCommand::TimerEvent:
    {
      iStyle->OnTimer();
      break;
    }
    case vtkCommand::KeyPressEvent:
    {
      keySym = iStyle->GetInteractor()->GetKeySym();
      if (keySymToCode.find(keySym) != keySymToCode.end())
      {
        keyCode = keySymToCode[keySym];
      }
      if (keyCode >= 0)
      {
        keyCode = keySymToCode[keySym];
        io.KeysDown[keyCode] = true;
        io.KeySuper = (keySym == "Win_L") || (keySym == "Win_R") || (keySym == "Meta_L") ||
          (keySym == "Meta_R");
      }

      if (!io.WantCaptureKeyboard || (io.WantCaptureKeyboard && self->GrabKeyboard))
      {
        iStyle->OnKeyDown();
        iStyle->OnKeyPress();
      }
      break;
    }
    case vtkCommand::KeyReleaseEvent:
    {
      keySym = iStyle->GetInteractor()->GetKeySym();
      if (keySymToCode.find(keySym) != keySymToCode.end())
      {
        keyCode = keySymToCode[keySym];
      }
      if (keyCode >= 0)
      {
        io.KeysDown[keyCode] = false;
        io.KeySuper = false;
      }

      if (!io.WantCaptureKeyboard || (io.WantCaptureKeyboard && self->GrabKeyboard))
      {
        iStyle->OnKeyUp();
        iStyle->OnKeyRelease();
      }
      break;
    }
    case vtkCommand::CharEvent:
    {
      io.AddInputCharacter(iStyle->GetInteractor()->GetKeyCode());

      if (!io.WantCaptureKeyboard || (io.WantCaptureKeyboard && self->GrabKeyboard))
      {
        iStyle->OnChar();
      }
      break;
    }
    default:
      break;
  }
}