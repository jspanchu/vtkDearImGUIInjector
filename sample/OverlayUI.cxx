// SPDX-FileCopyrightText: Copyright 2023 Jaswant Panchumarti
// SPDX-License-Identifier: BSD-3-Clause

#include "OverlayUI.h"
#include "vtkDearImGuiInjector.h"

#include <vtkInteractorObserver.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSetGet.h>

#ifdef ADOBE_IMGUI_SPECTRUM
#include "Karla-Regular.hpp"
#include "imgui_spectrum.h"
#endif
#include "imgui.h" // to draw custom UI

namespace
{
//------------------------------------------------------------------------------
void HelpMarker(const char* desc)
{
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}
}

//------------------------------------------------------------------------------
void OverlayUI::setup(vtkObject* caller, unsigned long, void*, void* callData)
{
  vtkDearImGuiInjector* overlay_ = reinterpret_cast<vtkDearImGuiInjector*>(caller);
  if (!callData)
  {
    return;
  }
  bool imguiInitStatus = *(reinterpret_cast<bool*>(callData));
  if (imguiInitStatus)
  {
    auto io = ImGui::GetIO();
#ifndef ADOBE_IMGUI_SPECTRUM
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(Karla_Regular_compressed_data_base85, 16);
    io.Fonts->AddFontDefault();
#else
    ImGui::Spectrum::LoadFont(18.0f);
    ImGui::Spectrum::StyleColorsSpectrum();
#endif
    auto& style = ImGui::GetStyle();
    style.ChildRounding = 8;
    style.FrameRounding = 8;
    style.GrabRounding = 8;
    style.PopupRounding = 8;
    style.ScrollbarRounding = 8;
    style.TabRounding = 8;
    style.WindowRounding = 8;
    style.FrameBorderSize = 1.f;
  }
  else
  {
    vtkErrorWithObjectMacro(
      overlay_, "Failed to setup overlay UI because ImGUI failed to initialize!");
  }
}

//------------------------------------------------------------------------------
void OverlayUI::draw(vtkObject* caller, unsigned long, void*, void* callData)
{
  vtkDearImGuiInjector* overlay_ = reinterpret_cast<vtkDearImGuiInjector*>(caller);

  ImGui::SetNextWindowBgAlpha(0.5);
  ImGui::SetNextWindowPos(ImVec2(5, 25), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(450, 550), ImGuiCond_Once);
  ImGui::Begin("VTK");
  if (ImGui::CollapsingHeader("vtkRenderWindow", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto rw = overlay_->Interactor->GetRenderWindow();
    ImGui::Text("MTime: %ld", rw->GetMTime());
    ImGui::Text("Name: %s", rw->GetClassName());
    if (ImGui::TreeNode("Capabilities"))
    {
      ImGui::TextWrapped("OpenGL: %s", rw->ReportCapabilities());
      ImGui::TreePop();
    }
  }
  if (ImGui::CollapsingHeader("vtkRenderWindowInteractor", ImGuiTreeNodeFlags_DefaultOpen))
  {
    auto& iren = overlay_->Interactor;
    ImGui::Text("MTime: %ld", iren->GetMTime());
    ImGui::Text("Name: %s", iren->GetClassName());
    if (ImGui::TreeNode("Style"))
    {
      auto styleBase = iren->GetInteractorStyle();
      vtkInteractorObserver* iStyle = nullptr;
      if (styleBase->IsA("vtkInteractorStyleSwitchBase"))
      {
        iStyle = vtkInteractorStyleSwitch::SafeDownCast(styleBase)->GetCurrentStyle();
      }
      else
      {
        iStyle = styleBase;
      }
      ImGui::Text("MTime: %ld", iStyle->GetMTime());
      ImGui::Text("Name: %s", iStyle->GetClassName());
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Mouse"))
    {
      int* xy = iren->GetEventPosition();
      ImGui::Text("X: %d", xy[0]);
      ImGui::Text("Y: %d", xy[1]);
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Keyboard"))
    {
      ImGui::Text("KeySym: %s", iren->GetKeySym());
      ImGui::SameLine();
      HelpMarker("VTK does not flush KeySym per frame.");
      ImGui::Text("KeyCode: %c", iren->GetKeyCode());
      ImGui::Text("Mods: %s %s %s", (iren->GetAltKey() ? "ALT" : " "),
        (iren->GetControlKey() ? "CTRL" : " "), (iren->GetShiftKey() ? "SHIFT" : " "));
      ImGui::TreePop();
    }
  }
  ImGui::End();
}
