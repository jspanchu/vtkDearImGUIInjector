#include "vtkDearImGUIInjector.h"
#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCameraOrientationRepresentation.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkContourFilter.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkObjectBase.h>
#include <vtkPerlinNoise.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSampleFunction.h>

#include "imgui.h"

int main(int, char*[])
{
  vtkNew<vtkNamedColors> colors;

  vtkNew<vtkPerlinNoise> perlinNoise;
  perlinNoise->SetFrequency(2, 1.25, 1.5);
  perlinNoise->SetPhase(0, 0, 0);

  vtkNew<vtkSampleFunction> sample;
  sample->SetImplicitFunction(perlinNoise);
  sample->SetSampleDimensions(32, 32, 32);
  sample->SetCapValue(1.);
  sample->ComputeNormalsOff();

  vtkNew<vtkContourFilter> surface;
  surface->SetInputConnection(sample->GetOutputPort());
  surface->SetValue(0, 0.0);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->ScalarVisibilityOff();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 0.276, 0.166);
  actor->GetProperty()->SetEdgeColor(0.902, 0.317, 0.217);
  actor->GetProperty()->SetDiffuse(1.);
  actor->GetProperty()->SetAmbient(1.);
  actor->GetProperty()->SetSpecular(1.);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  interactor->SetInteractorStyle(vtkNew<vtkInteractorStyleTrackballCamera>());

  // Add the actors to the renderer, set the background and size
  renderer->AddActor(actor);
  renderer->SetBackground(colors->GetColor3d("SlateGray").GetData());

  // Convenient camera manipulator.
  vtkNew<vtkCameraOrientationWidget> camManipulator;
  camManipulator->SetParentRenderer(renderer);

  // Initialize an overlay with DearImgui elements.
  vtkNew<vtkDearImGUIInjector> dearImGuiOverlay;
  dearImGuiOverlay->Inject(interactor);

  // Draw a set of imgui widgets to play with
  vtkNew<vtkCallbackCommand> imguiDrawCb;
  auto imguiDrawFn = [](vtkObject* caller, unsigned long, void* clientData, void* callData)
  {
    vtkActor* actor = nullptr;
    {
      auto obj = reinterpret_cast<vtkObjectBase*>(clientData);
      actor = vtkActor::SafeDownCast(obj);
    }
    if (actor == nullptr)
    {
      return;
    }
    auto mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
    if (mapper == nullptr)
    {
      return;
    }
    auto contour = vtkContourFilter::SafeDownCast(mapper->GetInputAlgorithm());
    if (contour == nullptr)
    {
      return;
    }
    auto sampleFunc = vtkSampleFunction::SafeDownCast(contour->GetInputAlgorithm());
    if (sampleFunc == nullptr)
    {
      return;
    }
    auto perlinNoise = vtkPerlinNoise::SafeDownCast(sampleFunc->GetImplicitFunction());
    if (perlinNoise == nullptr)
    {
      return;
    }

    double minV = VTK_DOUBLE_MIN;
    double maxV = VTK_DOUBLE_MAX;
    double zero = 0.;
    double one = 1.;

    auto& style = ImGui::GetStyle();
    style.ChildRounding = 8;
    style.FrameRounding = 8;
    style.GrabRounding = 8;
    style.PopupRounding = 8;
    style.ScrollbarRounding = 8;
    style.TabRounding = 8;
    style.WindowRounding = 8;
    style.FrameBorderSize = 1.f;

    ImGui::SetNextWindowBgAlpha(0.5);
    ImGui::SetNextWindowPos(ImVec2(0, 19), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(409, 633), ImGuiCond_Once);
    ImGui::Begin("Perlin Noise Example");

    auto io = ImGui::GetIO(); // for deltatime

    // PerlinNoise args
    static bool animate = true;
    static float speed[3] = { 1.f, 1.f, 1.f };
    static float offset[3] = { 1.56f, 3.12f, 3.28f };
    static double time = 0.;
    if (animate)
    {
      if (ImGui::Button("Pause"))
      {
        animate = false;
      }
    }
    else
    {
      animate = ImGui::Button("Play");
    }
    if (animate)
    {
      double phaseIn[3] = {};
      ImGui::DragFloat3("Speed", speed, 0.06f, 0.f, 10.0f);
      ImGui::DragFloat3("Offset", offset, 0.06f, 0.f, 10.0f);
      time += io.DeltaTime;
      perlinNoise->SetPhase(
        static_cast<double>(speed[0]) * std::sin(time + static_cast<double>(offset[0])),
        static_cast<double>(speed[1]) * std::sin(time + static_cast<double>(offset[1])),
        static_cast<double>(speed[2]) * std::sin(time + static_cast<double>(offset[2])));
    }
    if (ImGui::CollapsingHeader("Implicit Function", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool modified = ImGui::DragScalarN(
        "Frequency", ImGuiDataType_Double, perlinNoise->GetFrequency(), 3, 0.06f, &minV, &maxV);
      modified |= ImGui::DragScalarN(
        "Phase", ImGuiDataType_Double, perlinNoise->GetPhase(), 3, 0.06f, &minV, &maxV);
      if (modified)
      {
        perlinNoise->Modified();
      }
      double amp = perlinNoise->GetAmplitude();
      if (ImGui::DragScalar("Amp", ImGuiDataType_Double, &amp, 0.6f, &minV, &maxV))
      {
        perlinNoise->SetAmplitude(amp);
      }
      ImGui::LabelText("MTime", "%ld", perlinNoise->GetMTime());
    }

    // Sample function args
    if (ImGui::CollapsingHeader("Sample Function", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool modified =
        ImGui::DragInt3("Dimensions", sampleFunc->GetSampleDimensions(), 0.6f, 0, 1 << 16);
      modified |= ImGui::DragScalarN("XBounds", ImGuiDataType_Double, sampleFunc->GetModelBounds(),
        2, 0.06f, &minV, &maxV, "%f");
      modified |= ImGui::DragScalarN("YBounds", ImGuiDataType_Double,
        sampleFunc->GetModelBounds() + 2, 2, 0.06f, &minV, &maxV, "%f");
      modified |= ImGui::DragScalarN("ZBounds", ImGuiDataType_Double,
        sampleFunc->GetModelBounds() + 4, 2, 0.06f, &minV, &maxV, "%f");
      if (modified)
      {
        sampleFunc->Modified();
      }
      if (ImGui::RadioButton("Capping", sampleFunc->GetCapping()))
      {
        sampleFunc->SetCapping(!sampleFunc->GetCapping());
      }
      if (sampleFunc->GetCapping())
      {
        double capVal = sampleFunc->GetCapValue();
        if (ImGui::DragScalar("CapValue", ImGuiDataType_Double, &capVal, 0.06f, &minV, &maxV, "%f"))
        {
          sampleFunc->SetCapValue(capVal);
        }
      }
      ImGui::LabelText("MTime", "%ld", sampleFunc->GetMTime());
    }

    // Contour arguments
    if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen))
    {
      if (ImGui::DragScalar(
            "Value", ImGuiDataType_Double, contour->GetValues(), 0.06f, &minV, &maxV))
      {
        contour->Modified();
      }

      ImGui::LabelText("MTime", "%ld", contour->GetMTime());
    }

    // Visuals
    if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen))
    {
      double colord[4] = {};
      float colorf[4] = {};
      actor->GetProperty()->GetColor(colord);
      for (int i = 0; i < 3; ++i)
      {
        colorf[i] = static_cast<float>(colord[i]);
      }
      colorf[3] = static_cast<float>(actor->GetProperty()->GetOpacity());
      if (ImGui::ColorEdit4(
            "Prop", colorf, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags__OptionsDefault))
      {
        actor->GetProperty()->SetColor(colorf[0], colorf[1], colorf[2]);
        actor->GetProperty()->SetOpacity(colorf[3]);
      }
      actor->GetProperty()->GetEdgeColor(colord);
      for (int i = 0; i < 3; ++i)
      {
        colorf[i] = static_cast<float>(colord[i]);
      }
      if (ImGui::ColorEdit3(
            "Edge", colorf, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags__OptionsDefault))
      {
        actor->GetProperty()->SetEdgeColor(colorf[0], colorf[1], colorf[2]);
      }
      double diffuse = actor->GetProperty()->GetDiffuse();
      if (ImGui::DragScalar("Diffuse", ImGuiDataType_Double, &diffuse, 0.01f, &zero, &one))
      {
        actor->GetProperty()->SetDiffuse(diffuse);
      }
      double specular = actor->GetProperty()->GetSpecular();
      if (ImGui::DragScalar("Specular", ImGuiDataType_Double, &specular, 0.01f, &zero, &one))
      {
        actor->GetProperty()->SetSpecular(specular);
      }
      double ambient = actor->GetProperty()->GetAmbient();
      if (ImGui::DragScalar("Ambient", ImGuiDataType_Double, &ambient, 0.01f, &zero, &one))
      {
        actor->GetProperty()->SetAmbient(ambient);
      }
      if (ImGui::RadioButton("EdgeVisibility", actor->GetProperty()->GetEdgeVisibility()))
      {
        actor->GetProperty()->SetEdgeVisibility(!actor->GetProperty()->GetEdgeVisibility());
      }
      ImGui::LabelText("MTime", "%ld", actor->GetMTime());
    }

    ImGui::End();
  };

  imguiDrawCb->SetClientData(reinterpret_cast<void*>(actor.Get()));
  imguiDrawCb->SetCallback(imguiDrawFn);

  // Execute the draw function when it's time.
  dearImGuiOverlay->AddObserver(vtkDearImGUIInjector::ImGUIDrawEvent, imguiDrawCb);

  renderWindow->SetWindowName("PerlinNoise");
  renderWindow->SetSize(1920, 1080);
  renderer->ResetCamera();

  camManipulator->On();
  renderWindow->Render();
  auto rep = vtkCameraOrientationRepresentation::SafeDownCast(camManipulator->GetRepresentation());
  rep->SetPadding(0, 30);

  interactor->Start();

  return EXIT_SUCCESS;
}