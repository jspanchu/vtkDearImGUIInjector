#include <vtkNew.h>
#include <vtkObject.h>
#include <vtkWeakPointer.h>
#include <vtkdearimguiinjector_export.h>

class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkCallbackCommand;
class vtkInteractorStyle;

class VTKDEARIMGUIINJECTOR_EXPORT vtkDearImGUIInjector : public vtkObject
{
public:
  static vtkDearImGUIInjector* New();
  vtkTypeMacro(vtkDearImGUIInjector, vtkObject);

  // rendering and interaction callbacks are installed here.
  void Inject(vtkRenderWindowInteractor* interactor);

  void InstallEventCallback(vtkRenderWindowInteractor* interactor);
  void UninstallEventCallback();

  // Observe this event and draw application specific ImGUI widgets
  static const unsigned long ImGUIDrawEvent;
  vtkWeakPointer<vtkRenderWindowInteractor> Interactor;

protected:
  vtkDearImGUIInjector();
  ~vtkDearImGUIInjector() override;

  // pair imgui with vtk
  bool SetUp(vtkRenderWindow* renWin);
  void TearDown(vtkObject* caller, unsigned long eid, void* callData);

  // hooks into vtkRenderWindow
  void BeginDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* callData);
  void RenderDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* callData);

  // Mouse will be set here.
  void UpdateMousePosAndButtons(vtkRenderWindowInteractor* interactor);
  void UpdateMouseCursor(vtkRenderWindow* renWin);

  // Run the event loop.
  void PumpEv(vtkObject* caller, unsigned long eid, void* callData);

  // routes events:
  // VTK[X,Win32,Cocoa]Interactor >>>> DearImGUI >>>> VTK[...]InteractorStyle
  static void DispatchEv(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

  vtkNew<vtkCallbackCommand> EventCallbackCommand;
  vtkWeakPointer<vtkInteractorStyle> currentIStyle;

  float Time = 0;
  bool MouseJustPressed[3] = { false, false, false };
  bool FinishedSetup = false;
  bool Focused = true;
  bool GrabMouse = false;    // true: pass mouse to vtk, false: imgui accepts mouse
                             // and doesn't give it to VTK (when ui is focused)
  bool GrabKeyboard = false; // true: pass keys to vtk, false: imgui accepts
                             // keys and doesn't give it to VTK (when ui is focused)

  bool ShowDemo = false;
  bool ShowAppMetrics = false;
  bool ShowAppStyleEditor = false;
  bool ShowAppAbout = false;

private:
  vtkDearImGUIInjector(const vtkDearImGUIInjector&) = delete;
  void operator=(const vtkDearImGUIInjector&) = delete;
};
