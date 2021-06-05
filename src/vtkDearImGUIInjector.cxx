#include <vtkDearImGUIInjector.h>

#include <chrono>
#include <string>
#include <unordered_map>

#include <vtkCallbackCommand.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleSwitch.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

vtkStandardNewMacro(vtkDearImGUIInjector);

class InteractionEventsHelper
{
  std::vector<unsigned long> Tags;
  vtkSmartPointer<vtkInteractorStyle> Target;
public:
  vtkInteractorStyle* GetTarget() { return Target; }

  void Observe(vtkInteractorStyle* iStyle, vtkDearImGUIInjector* observerObj)
  {
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::LeftButtonPressEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::RightButtonPressEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MiddleButtonPressEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::RightButtonReleaseEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MiddleButtonReleaseEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MouseWheelBackwardEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MouseWheelForwardEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MouseWheelLeftEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::MouseWheelRightEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::KeyPressEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::KeyReleaseEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::CharEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::EnterEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    this->Tags.push_back(iStyle->AddObserver(vtkCommand::LeaveEvent, observerObj, &vtkDearImGUIInjector::Intercept));
    iStyle->HandleObserversOn();
    this->Target = iStyle;
  }

  void Ignore()
  {
    if (!this->Target)
    {
      return;
    }
    for (const auto& tag : Tags)
    {
      this->Target->RemoveObserver(tag);
    }
    this->Target = nullptr;
  }
};

namespace
{
  vtkInteractorStyle* GetInteractorStyle(vtkRenderWindowInteractor* interactor)
  {
    auto styleSwitch = vtkInteractorStyleSwitch::SafeDownCast(interactor->GetInteractorStyle());
    vtkInteractorStyle* iStyle = nullptr;
    if (!styleSwitch)
    {
      iStyle = vtkInteractorStyle::SafeDownCast(interactor->GetInteractorStyle());
    }
    else
    {
      iStyle = styleSwitch->GetCurrentStyle();
    }
    return iStyle;
  }

  std::unordered_map<std::string, int> keySymToCode({
    {"Tab", 0},
    {"LeftArrow", 1},
    {"RightArrow", 2},
    {"UpArrow", 3},
    {"DownArrow", 4},
    {"PageUp", 5},
    {"PageDown", 6},
    {"Home", 7},
    {"End", 8},
    {"Insert", 9},
    {"Delete", 10},
    {"Backspace", 11},
    {"Space", 12},
    {"Enter", 13},
    {"Escape", 14},
    {"KeyPadEnter", 15},
    {"A", 16},
    {"C", 17},
    {"V", 18},
    {"X", 19},
    {"Y", 20},
    {"Z", 21},
    {"a", 16},
    {"c", 17},
    {"v", 18},
    {"x", 19},
    {"y", 20},
    {"z", 21}
    });

  const std::unordered_map<int, int> imguiToVtkCursors({
    {ImGuiMouseCursor_None, VTK_CURSOR_DEFAULT},
    {ImGuiMouseCursor_Arrow, VTK_CURSOR_ARROW},
    {ImGuiMouseCursor_TextInput, VTK_CURSOR_DEFAULT},
    {ImGuiMouseCursor_ResizeAll, VTK_CURSOR_SIZEALL},
    {ImGuiMouseCursor_ResizeNS, VTK_CURSOR_SIZENS},
    {ImGuiMouseCursor_ResizeEW, VTK_CURSOR_SIZEWE},
    {ImGuiMouseCursor_ResizeNESW, VTK_CURSOR_SIZENE},
    {ImGuiMouseCursor_ResizeNWSE, VTK_CURSOR_SIZENW},
    {ImGuiMouseCursor_Hand, VTK_CURSOR_HAND},
    {ImGuiMouseCursor_NotAllowed, VTK_CURSOR_DEFAULT}
    });
}

vtkDearImGUIInjector::vtkDearImGUIInjector()
{
  // Start ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  this->EventHelper = new InteractionEventsHelper();
}

vtkDearImGUIInjector::~vtkDearImGUIInjector()
{
  // Safely exit when VTK app exits
  if (ImGui::GetCurrentContext())
  {
    ImGui::DestroyContext();
  }
  delete this->EventHelper;
  this->EventHelper = nullptr;
}

void vtkDearImGUIInjector::Inject(vtkRenderWindowInteractor* interactor)
{
  //this->DebugOn();
  auto renWin = interactor->GetRenderWindow();
  if (!renWin)
  {
    vtkWarningMacro(<< "Interactor does not have a render window!");
    return;
  }

  // intercept renderer
  renWin->AddObserver(vtkCommand::StartEvent, this, &vtkDearImGUIInjector::BeginDearImGUIOverlay);
  renWin->AddObserver(vtkCommand::RenderEvent, this, &vtkDearImGUIInjector::RenderDearImGUIOverlay);

  // intercept interactor
  auto iStyle = GetInteractorStyle(interactor);
  this->EventHelper->Observe(iStyle, this);
  interactor->AddObserver(vtkCommand::TimerEvent, this, &vtkDearImGUIInjector::Render);

  // Safely exit when vtk app exits
  interactor->AddObserver(vtkCommand::ExitEvent, this, &vtkDearImGUIInjector::ShutDown);
}

bool vtkDearImGUIInjector::Init(vtkRenderWindow* renWin)
{
  if (renWin->GetNeverRendered())
  {
    vtkDebugMacro(<< "Init called, but render library is not setup. Hold on..");
    return false; // too early
  }
  if (!this->Time)
  {
    using namespace std::chrono;
    this->Time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  }

  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

  // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
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
  io.KeyMap[ImGuiKey_Backspace] = keySymToCode["Backspace"];
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

  ImGui_ImplOpenGL3_Init();

  // render everything each time in the event loop
  this->RenderTimerId = renWin->GetInteractor()->CreateOneShotTimer(0);

#if defined(_WIN32)
  io.BackendPlatformName = renWin->GetClassName();
  io.ImeWindowHandle = renWin->GetGenericWindowId();
#endif
  return true;
}

void vtkDearImGUIInjector::BeginDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* calldata)
{
  vtkDebugMacro(<< "BeginDearImGUIOverlay");
  auto renWin = vtkRenderWindow::SafeDownCast(caller);
  if (!ImGui::GetCurrentContext())
  {
    vtkDebugMacro(<< "BeginDearImGUIOverlay called, but interactor has not yet started.");
    return; // wait for interactor to start
  }
  if (!this->Init(renWin))
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

  // Setup time step
  using namespace std::chrono;
  auto currentTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  io.DeltaTime = currentTime > 0 ? (currentTime - this->Time) : (1.0f / 60.0f);
  io.DeltaTime /= 1000.0f;
  this->Time = currentTime;

  auto interactor = renWin->GetInteractor();
  this->UpdateMousePosAndButtons(interactor);
  this->UpdateMouseCursor(renWin);

  auto iStyle = GetInteractorStyle(interactor);
  if (this->EventHelper->GetTarget() != iStyle)
  {
    this->EventHelper->Ignore();
    this->EventHelper->Observe(iStyle, this);
  }

  // Begin ImGUI drawing
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow((bool*)1);
}

void vtkDearImGUIInjector::RenderDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* calldata)
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
      (openGLrenWin->GetStereoRender() && openGLrenWin->GetStereoType() == VTK_STEREO_CRYSTAL_EYES) ? 1 : 0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    fbo->UnBind();
  }
}

void vtkDearImGUIInjector::ShutDown(vtkObject* caller, unsigned long eid, void* calldata)
{
  ImGui_ImplOpenGL3_Shutdown();
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto renWin = interactor->GetRenderWindow();
  renWin->GetInteractor()->DestroyTimer(this->RenderTimerId);
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
  const ImVec2 mouse_pos_backup = io.MousePos;
  io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
#ifdef __EMSCRIPTEN__
  this->Focused = true; // Emscripten
#endif
  if (this->Focused)
  {
    if (io.WantSetMousePos)
    {
      int xpos = static_cast<int>(mouse_pos_backup.x) - static_cast<int>(ImGui::GetWindowPos().x);
      int ypos = static_cast<int>(mouse_pos_backup.y) - static_cast<int>(ImGui::GetWindowPos().y);
      interactor->SetEventPositionFlipY(xpos, ypos);
    }
    else
    {
      int mouse_x, mouse_y;
      interactor->GetLastEventPosition(mouse_x, mouse_y);
      io.MousePos = ImVec2((float)mouse_x, io.DisplaySize.y - (float)mouse_y);
    }
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
    // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
    renWin->HideCursor();
  }
  else
  {
    // Show OS mouse cursor
    renWin->SetCurrentCursor(imguiToVtkCursors.at(imgui_cursor));
    renWin->ShowCursor();
  }
}

void vtkDearImGUIInjector::Intercept(vtkObject* caller, unsigned long eid, void* calldata)
{
  vtkDebugMacro(<< "Intercept");
  auto iStyle = vtkInteractorStyle::SafeDownCast(caller);
  auto interactor = iStyle->GetInteractor();

  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.KeyAlt = interactor->GetAltKey();
  io.KeyCtrl = interactor->GetControlKey();
  io.KeyShift = interactor->GetShiftKey();

  std::string keySym = "";
  int keyCode = -1;
  switch (eid)
  {
  case vtkCommand::LeftButtonPressEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Left] = true;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnLeftButtonDown();
    }
    break;
  }
  case vtkCommand::RightButtonPressEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Right] = true;
    if (!io.WantCaptureMouse)
    { 
      iStyle->OnRightButtonDown(); 
    }
    break;
  }
  case vtkCommand::MiddleButtonPressEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Middle] = true;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnMiddleButtonDown();
    }
    break;
  }
  case vtkCommand::LeftButtonReleaseEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Left] = false;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnLeftButtonUp();
    }
    break;
  }
  case vtkCommand::RightButtonReleaseEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Right] = false;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnRightButtonUp();
    }
    break;
  }
  case vtkCommand::MiddleButtonReleaseEvent:
  {
    this->MouseJustPressed[ImGuiMouseButton_Middle] = false;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnMiddleButtonUp();
    }
    break;
  }
  case vtkCommand::MouseWheelBackwardEvent:
  {
    io.MouseWheel = -1;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnMouseWheelBackward();
    }
    break;
  }
  case vtkCommand::MouseWheelForwardEvent:
    io.MouseWheel = 1;
    {
      if (!io.WantCaptureMouse)
      {
        iStyle->OnMouseWheelForward();
      }
      break;
    }
  case vtkCommand::MouseWheelLeftEvent:
  {
    io.MouseWheelH = 1;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnMouseWheelLeft();
    }
    break;
  }
  case vtkCommand::MouseWheelRightEvent:
  {
    io.MouseWheelH = -1;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnMouseWheelRight();
    }
    break;
  }
  case vtkCommand::KeyPressEvent:
  {
    keySym = interactor->GetKeySym();
    if (keySymToCode.find(keySym) != keySymToCode.end())
    {
      keyCode = keySymToCode[keySym];
    }
    if (keyCode >= 0)
    {
      keyCode = keySymToCode[keySym];
      io.KeysDown[keyCode] = true;
      io.KeySuper = (keySym == "Win_L") || (keySym == "Win_R") || (keySym == "Meta_L") || (keySym == "Meta_R");
    }
    if (!io.WantCaptureKeyboard)
    {
      iStyle->OnKeyDown();
      iStyle->OnKeyPress();
    }
    break;
  }
  case vtkCommand::KeyReleaseEvent:
  {
    keySym = interactor->GetKeySym();
    if (keySymToCode.find(keySym) != keySymToCode.end())
    {
      keyCode = keySymToCode[keySym];
    }
    if (keyCode >= 0)
    {
      io.KeysDown[keyCode] = false;
      io.KeySuper = false;
    }
    if (!io.WantCaptureKeyboard)
    {
      iStyle->OnKeyUp();
      iStyle->OnKeyRelease();
    }
    break;
  }
  case vtkCommand::CharEvent:
  {
    if (!io.WantCaptureKeyboard)
    {
      iStyle->OnChar();
    }
    break;
  }
  case vtkCommand::EnterEvent:
  {
    this->Focused = true;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnEnter();
    }
    break;
  }
  case vtkCommand::LeaveEvent:
  {
    this->Focused = false;
    if (!io.WantCaptureMouse)
    {
      iStyle->OnLeave();
    }
    break;
  }
  default:
    break;
  }
}

void vtkDearImGUIInjector::Render(vtkObject* caller, unsigned long eid, void* calldata)
{
  vtkDebugMacro(<< "Render Now");
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);

  switch (eid)
  {
  case vtkCommand::TimerEvent:
    if (interactor->GetVTKTimerId(*reinterpret_cast<int*>(calldata)) == this->RenderTimerId)
    {
      interactor->Render();
      interactor->ResetTimer(this->RenderTimerId);
    }
  default:
    break;
  }
}