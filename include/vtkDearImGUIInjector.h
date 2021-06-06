#include <vtkObject.h>

#include <vtkdearimguiinjector_export.h>

class vtkInteractorStyle;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class InteractionEventsHelper;

class VTKDEARIMGUIINJECTOR_EXPORT vtkDearImGUIInjector : public vtkObject
{
public:
  static vtkDearImGUIInjector* New();
  vtkTypeMacro(vtkDearImGUIInjector, vtkObject);

  // rendering and interaction callbacks are installed here.
  void Inject(vtkRenderWindowInteractor* interactor);

  // routes interactor events this way: VTK >>>> DearImGUI
  void Intercept(vtkObject* caller, unsigned long eid, void* calldata);

protected:
  vtkDearImGUIInjector();
  ~vtkDearImGUIInjector() override;

  bool SetUp(vtkRenderWindow* renWin);
  void TearDown(vtkObject* caller, unsigned long eid, void* calldata);
  void UpdateMousePosAndButtons(vtkRenderWindowInteractor* interactor);
  void UpdateMouseCursor(vtkRenderWindow* renWin);

  // routes Render event this way: VTK >>>> DearImGUI
  void EventLoop(vtkObject* caller, unsigned long eid, void* calldata);

  // hook into vtkRenderWindow
  void BeginDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* calldata);
  void RenderDearImGUIOverlay(vtkObject* caller, unsigned long eid, void* calldata);

  InteractionEventsHelper* EventHelper;
  unsigned long long Time = 0;
  bool MouseJustPressed[3] = { false, false, false };
  bool Focused = false;
  bool GrabMouse = false; // false: pass mouse to vtk, true: do not pass mouse to vtk

private:
  vtkDearImGUIInjector(const vtkDearImGUIInjector&) = delete;
  void operator=(const vtkDearImGUIInjector&) = delete;
};
