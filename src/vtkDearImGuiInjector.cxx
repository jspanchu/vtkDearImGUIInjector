// SPDX-FileCopyrightText: Copyright 2023 Jaswant Panchumarti
// SPDX-License-Identifier: BSD-3-Clause

#include <chrono>
#include <string>
#include <unordered_map>

#include <vtkDearImGuiInjector.h>

#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkObject.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtk_glew.h>

#if defined(GL_ES_VERSION_3_0) && !defined(IMGUI_IMPL_OPENGL_ES3)
#define IMGUI_IMPL_OPENGL_ES3
#endif

#include "imgui_impl_opengl3.h"
#include "imgui.h"

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#include <emscripten/html5.h>
#endif

#ifdef USES_X11
#include <X11/Xlib.h>
#elif defined(USES_SDL2)
#include <SDL2/SDL.h>
#elif defined(USES_WIN32)
#include <vtkWindows.h>
#endif

vtkStandardNewMacro(vtkDearImGuiInjector);

namespace
{
#ifdef USES_WIN32
std::unordered_map<std::string, unsigned int> KeySymToVKeyCode = { { "Cancel", 3 },
  { "BackSpace", 8 }, { "Tab", 9 }, { "Clear", 12 }, { "Return", 13 }, { "Shift_L", 16 },
  { "Control_L", 17 }, { "Alt_L", 18 }, { "Pause", 19 }, { "Caps_Lock", 20 }, { "Escape", 27 },
  { "space", 32 }, { "Prior", 33 }, { "Next", 34 }, { "End", 35 }, { "Home", 36 }, { "Left", 37 },
  { "Up", 38 }, { "Right", 39 }, { "Down", 40 }, { "Select", 41 }, { "Execute", 43 },
  { "Snapshot", 44 }, { "Insert", 45 }, { "Delete", 46 }, { "Help", 47 }, { "1", 49 }, { "2", 50 },
  { "3", 51 }, { "4", 52 }, { "5", 53 }, { "6", 54 }, { "7", 55 }, { "8", 56 }, { "9", 57 },
  { "a", 65 }, { "b", 66 }, { "c", 67 }, { "d", 68 }, { "e", 69 }, { "f", 70 }, { "g", 71 },
  { "h", 72 }, { "i", 73 }, { "j", 74 }, { "k", 75 }, { "l", 76 }, { "m", 77 }, { "n", 78 },
  { "o", 79 }, { "p", 80 }, { "q", 81 }, { "r", 82 }, { "s", 83 }, { "t", 84 }, { "u", 85 },
  { "v", 86 }, { "w", 87 }, { "x", 88 }, { "y", 89 }, { "z", 90 }, { "Win_L", 91 }, { "Win_R", 92 },
  { "App", 93 }, { "KP_0", 96 }, { "KP_1", 97 }, { "KP_2", 98 }, { "KP_3", 99 }, { "KP_4", 100 },
  { "KP_5", 101 }, { "KP_6", 102 }, { "KP_7", 103 }, { "KP_8", 104 }, { "KP_9", 105 },
  { "asterisk", 106 }, { "plus", 107 }, { "bar", 108 }, { "minus", 109 }, { "period", 110 },
  { "slash", 111 }, { "F1", 112 }, { "F2", 113 }, { "F3", 114 }, { "F4", 115 }, { "F5", 116 },
  { "F6", 117 }, { "F7", 118 }, { "F8", 119 }, { "F9", 120 }, { "F10", 121 }, { "F11", 122 },
  { "F12", 123 }, { "F13", 124 }, { "F14", 125 }, { "F15", 126 }, { "F16", 127 }, { "F17", 128 },
  { "F18", 129 }, { "F19", 130 }, { "F20", 131 }, { "F21", 132 }, { "F22", 133 }, { "F23", 134 },
  { "F24", 135 }, { "Num_Lock", 144 }, { "Scroll_Lock", 145 } };
#endif

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

//------------------------------------------------------------------------------
vtkDearImGuiInjector::vtkDearImGuiInjector()
{
  // Start DearImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
}

//------------------------------------------------------------------------------
vtkDearImGuiInjector::~vtkDearImGuiInjector()
{
  // Destroy DearImGUi
  if (ImGui::GetCurrentContext())
  {
    ImGui::DestroyContext();
  }
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::Inject(vtkRenderWindowInteractor* interactor)
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
  this->EventInterceptor->SetClientData(this);
  this->EventInterceptor->SetCallback(&vtkDearImGuiInjector::InterceptEvent);
  this->Interactor->AddObserver(vtkCommand::StartEvent, this, &vtkDearImGuiInjector::PumpEvents);

  // intercept renderer
  renWin->AddObserver(vtkCommand::StartEvent, this, &vtkDearImGuiInjector::BeginDearImGuiOverlay);
  renWin->AddObserver(vtkCommand::RenderEvent, this, &vtkDearImGuiInjector::RenderDearImGuiOverlay);

  // Safely exit when vtk app exits
  this->Interactor->AddObserver(vtkCommand::ExitEvent, this, &vtkDearImGuiInjector::TearDown);
}

//------------------------------------------------------------------------------
bool vtkDearImGuiInjector::SetUp(vtkRenderWindow* renWin)
{
  if (renWin->GetNeverRendered())
  {
    vtkDebugMacro(<< "Init called, but render library is not setup. Hold on..");
    return false; // too early
  }
  if (this->FinishedSetup)
  {
    return true;
  }

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values
                                                        // (optional)

  // Keyboard mapping. Dear ImGui will use those indices to peek into the
  // io.KeysDown[] array.
#ifdef USES_X11
  io.KeyMap[ImGuiKey_Tab] = XStringToKeysym("Tab") & 0xff;
  io.KeyMap[ImGuiKey_LeftArrow] = XStringToKeysym("Left") & 0xff;
  io.KeyMap[ImGuiKey_RightArrow] = XStringToKeysym("Right") & 0xff;
  io.KeyMap[ImGuiKey_UpArrow] = XStringToKeysym("Up") & 0xff;
  io.KeyMap[ImGuiKey_DownArrow] = XStringToKeysym("Down") & 0xff;
  io.KeyMap[ImGuiKey_PageUp] = XStringToKeysym("Page_Up") & 0xff;
  io.KeyMap[ImGuiKey_PageDown] = XStringToKeysym("Page_Down") & 0xff;
  io.KeyMap[ImGuiKey_Home] = XStringToKeysym("Home") & 0xff;
  io.KeyMap[ImGuiKey_End] = XStringToKeysym("End") & 0xff;
  io.KeyMap[ImGuiKey_Insert] = XStringToKeysym("Insert") & 0xff;
  io.KeyMap[ImGuiKey_Delete] = XStringToKeysym("Delete") & 0xff;
  io.KeyMap[ImGuiKey_Backspace] = XStringToKeysym("BackSpace") & 0xff;
  io.KeyMap[ImGuiKey_Space] = XStringToKeysym("Space") & 0xff;
  io.KeyMap[ImGuiKey_Enter] = XStringToKeysym("Return") & 0xff;
  io.KeyMap[ImGuiKey_Escape] = XStringToKeysym("Escape") & 0xff;
  io.KeyMap[ImGuiKey_KeyPadEnter] = XStringToKeysym("KP_Enter") & 0xff;
  io.KeyMap[ImGuiKey_A] = XStringToKeysym("A") & 0xff;
  io.KeyMap[ImGuiKey_C] = XStringToKeysym("C") & 0xff;
  io.KeyMap[ImGuiKey_V] = XStringToKeysym("V") & 0xff;
  io.KeyMap[ImGuiKey_X] = XStringToKeysym("X") & 0xff;
  io.KeyMap[ImGuiKey_Y] = XStringToKeysym("Y") & 0xff;
  io.KeyMap[ImGuiKey_Z] = XStringToKeysym("Z") & 0xff;
#elif USES_SDL2
  io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
  io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
  io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
  io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
  io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
  io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
  io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
  io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
  io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
  io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
  io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
  io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
  io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
  io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
  io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
#elif USES_WIN32
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';
#endif
#if defined(_WIN32)
  io.BackendPlatformName = renWin->GetClassName();
  io.ImeWindowHandle = renWin->GetGenericWindowId();
#endif
  bool status = ImGui_ImplOpenGL3_Init();
  this->FinishedSetup = status;
  this->InvokeEvent(
    vtkDearImGuiInjector::ImGuiSetupEvent, reinterpret_cast<void*>(&this->FinishedSetup));
  return status;
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::TearDown(vtkObject* caller, unsigned long eid, void* callData)
{
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  if (interactor != nullptr)
  {
    interactor->SetDone(true);
  }
  ImGui_ImplOpenGL3_Shutdown();
  this->InvokeEvent(vtkDearImGuiInjector::ImGuiTearDownEvent, nullptr);
  vtkDebugMacro(<< "tear down");
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::BeginDearImGuiOverlay(
  vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "BeginDearImGuiOverlay");
  auto renWin = vtkRenderWindow::SafeDownCast(caller);

  // Ensure valid DearImGui context exists.
  if (!ImGui::GetCurrentContext())
  {
    vtkDebugMacro(<< "BeginDearImGuiOverlay called, but DearImGui context does not exist.");
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

  // Increment time for DearImGui
  using namespace std::chrono;
  auto currentTime =
    double(duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()) /
    1000.;
  io.DeltaTime =
    (this->Time > 0.0 && this->Time < currentTime) ? (currentTime - this->Time) : (1. / 60.);
  this->Time = currentTime;

  auto interactor = renWin->GetInteractor();
  this->UpdateMousePosAndButtons(interactor);
  this->UpdateMouseCursor(renWin);

  vtkDebugMacro(<< "new frame begin");

  // Begin ImGui drawing
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
  // Menu Bar
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Input"))
    {
      ImGui::MenuItem("Grab Mouse", NULL, &this->GrabMouse);
      ImGui::MenuItem("Grab Keyboard", NULL, &this->GrabKeyboard);
      ImGui::MenuItem("Hardware Cursor", NULL, &io.MouseDrawCursor);
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
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
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
  this->InvokeEvent(ImGuiDrawEvent);

  vtkDebugMacro(<< "new frame end");
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::RenderDearImGuiOverlay(
  vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "RenderDearImGuiOverlay");
  auto renWin = vtkRenderWindow::SafeDownCast(caller);
  auto openGLrenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  ImGuiIO& io = ImGui::GetIO();
  if (io.Fonts->IsBuilt())
  {
    ImGui::Render();
    auto fbo = openGLrenWin->GetRenderFramebuffer();
    fbo->Bind();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    fbo->UnBind();
  }
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::InstallEventCallback(vtkRenderWindowInteractor* interactor)
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

  this->CurrentIStyle = nullptr;
  if (styleBase->IsA("vtkInteractorStyleSwitchBase"))
  {
    this->CurrentIStyle = vtkInteractorStyleSwitch::SafeDownCast(styleBase)->GetCurrentStyle();
  }
  else
  {
    this->CurrentIStyle = styleBase;
  }

  this->CurrentIStyle->AddObserver(vtkCommand::EnterEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::LeaveEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::MouseMoveEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::LeftButtonPressEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::LeftButtonReleaseEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::LeftButtonDoubleClickEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MiddleButtonPressEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MiddleButtonReleaseEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MiddleButtonDoubleClickEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::RightButtonPressEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::RightButtonReleaseEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::RightButtonDoubleClickEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MouseWheelForwardEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MouseWheelBackwardEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MouseWheelLeftEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(
    vtkCommand::MouseWheelRightEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::ExposeEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::ConfigureEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::TimerEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::KeyPressEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::KeyReleaseEvent, this->EventInterceptor, 1.0);

  this->CurrentIStyle->AddObserver(vtkCommand::CharEvent, this->EventInterceptor, 1.0);
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::UninstallEventCallback()
{
  this->CurrentIStyle->RemoveObserver(this->EventInterceptor);
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::UpdateMousePosAndButtons(vtkRenderWindowInteractor* interactor)
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

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::UpdateMouseCursor(vtkRenderWindow* renWin)
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

namespace
{

//------------------------------------------------------------------------------
void mainLoopCallback(void* arg)
{
  vtkDearImGuiInjector* self = static_cast<vtkDearImGuiInjector*>(arg);
  vtkRenderWindowInteractor* interactor = self->Interactor;
  vtkRenderWindow* renWin = interactor->GetRenderWindow();

  self->InstallEventCallback(interactor);
  interactor->ProcessEvents();
  self->UninstallEventCallback();
  if (!interactor->GetDone())
    renWin->Render();
}

#ifdef __EMSCRIPTEN__
EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent* e, void* userData)
{
  auto interactor = reinterpret_cast<vtkRenderWindowInteractor*>(userData);
  interactor->UpdateSize(e->windowInnerWidth, e->windowInnerHeight);
  return 0;
}
#endif

}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::PumpEvents(vtkObject* caller, unsigned long eid, void* callData)
{
  vtkDebugMacro(<< "PumpEvents");
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  interactor->Enable();
  interactor->Initialize();

#ifdef __EMSCRIPTEN__
  emscripten_set_resize_callback(
    EMSCRIPTEN_EVENT_TARGET_WINDOW, reinterpret_cast<void*>(interactor), 1, resizeCallback);
  emscripten_set_main_loop_arg(&mainLoopCallback, (void*)this, 0, vtkRenderWindowInteractor::InteractorManagesTheEventLoop);
#else
  while (!interactor->GetDone())
  {
    mainLoopCallback(reinterpret_cast<void*>(this));
  }
#endif
}

//------------------------------------------------------------------------------
void vtkDearImGuiInjector::InterceptEvent(
  vtkObject* caller, unsigned long eid, void* clientData, void* callData)
{
  // auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto iStyle = vtkInteractorStyle::SafeDownCast(caller);
  auto self = reinterpret_cast<vtkDearImGuiInjector*>(clientData);

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
    case vtkCommand::CharEvent:
    {
      std::string keySym = iStyle->GetInteractor()->GetKeySym();
#ifdef USES_WIN32
      unsigned int key = 0;
      if (KeySymToVKeyCode.find(keySym.c_str()) != KeySymToVKeyCode.end())
      {
        key = KeySymToVKeyCode.at(keySym.c_str());
      }
      else
      {
        key = static_cast<unsigned int>(iStyle->GetInteractor()->GetKeyCode());
      }
      io.AddInputCharacter(key);
#else
      io.AddInputCharactersUTF8(keySym.c_str());
#endif

      if (!io.WantCaptureKeyboard || (io.WantCaptureKeyboard && self->GrabKeyboard))
      {
        iStyle->OnChar();
      }
      break;
    }
    case vtkCommand::KeyPressEvent:
    case vtkCommand::KeyReleaseEvent:
    {
      bool down = eid == vtkCommand::KeyPressEvent;
      std::string keySym = iStyle->GetInteractor()->GetKeySym();
#ifdef USES_X11
      unsigned int key = iStyle->GetInteractor()->GetKeyCode();
      // Do not rely on VTK giving correct info for ctrl, shift, alt keys on X11.
      // So, check for literal in key sym string
      const auto& nul = std::string::npos;
      io.KeyAlt = (keySym.find("Alt") != nul) || (keySym.find("alt") != nul);
      io.KeyCtrl = (keySym.find("Control") != nul) || (keySym.find("control") != nul);
      io.KeyShift = (keySym.find("Shift") != nul) || (keySym.find("shift") != nul);
      io.KeySuper = (keySym == "Win_L") || (keySym == "Win_R") || (keySym == "Super_L") ||
        (keySym == "Super_R");
      io.KeySuper &= down;
#elif defined(USES_SDL2)
      unsigned int key =
        SDL_GetScancodeFromKey(static_cast<unsigned int>(iStyle->GetInteractor()->GetKeyCode()));
      if (!key)
      {
        key = static_cast<unsigned int>(iStyle->GetInteractor()->GetKeyCode());
      }
      io.KeyAlt = iStyle->GetInteractor()->GetAltKey();
      io.KeyCtrl = iStyle->GetInteractor()->GetControlKey();
      io.KeyShift = iStyle->GetInteractor()->GetShiftKey();
      io.KeySuper = (SDL_GetModState() & KMOD_GUI) ? true : false;
#elif defined(USES_WIN32)
      unsigned int key = 0;
      if (KeySymToVKeyCode.find(keySym.c_str()) != KeySymToVKeyCode.end())
      {
        key = KeySymToVKeyCode.at(keySym.c_str());
      }
      else
      {
        key = static_cast<unsigned int>(iStyle->GetInteractor()->GetKeyCode());
      }
      io.KeyAlt = iStyle->GetInteractor()->GetAltKey();
      io.KeyCtrl = iStyle->GetInteractor()->GetControlKey();
      io.KeyShift = iStyle->GetInteractor()->GetShiftKey();
      io.KeySuper = (GetKeyState(VK_LWIN) || GetKeyState(VK_RWIN)) ? true : false;
#endif
      if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown))
      {
        io.KeysDown[key] = (down);
      }
      io.KeyAlt &= down;
      io.KeyCtrl &= down;
      io.KeyShift &= down;
      io.KeySuper &= down;

      if (!io.WantCaptureKeyboard || (io.WantCaptureKeyboard && self->GrabKeyboard))
      {
        if (down)
        {
          iStyle->OnKeyDown();
          iStyle->OnKeyPress();
        }
        else
        {
          iStyle->OnKeyUp();
          iStyle->OnKeyRelease();
        }
      }
      break;
    }
    default:
      break;
  }
}
