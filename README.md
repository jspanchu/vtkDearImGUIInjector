# vtkDearImGUIInjector
[![Deploy WebAssembly binary](https://github.com/jspanchu/vtkDearImGUIInjector/actions/workflows/build-wasm.yml/badge.svg)](https://github.com/jspanchu/vtkDearImGUIInjector/actions/workflows/build-wasm.yml)

Inject DearImGUI into an existing VTK application.

A very closely related project is phcerdan's https://github.com/phcerdan/vtkImGuiAdapter.
The two are slightly different in that:
1. With vtkDearImGUIInjector, 
    
    a. you do not have to take care of ImGUI contexts yourself.
    
    b. no need to write an event loop.
2. With vtkImGUIAdapter,
    
    a. you are responsible for ensuring the sdl2 window is setup and the ImGUI context is initialized.
    
    b. you must write an event loop in your application, ensure events are processed and ensure NewFrame/EndFrame are called.

## Problems problems problems ...
On the web with emscripten, mouse input and all GL features upto GLES3 work. But, keyboard events do not work.
VTK lacks a standard key map definition and in all cases (X11, Win32, SDL2, Cocoa) the interactor yields a platform specific scancode.
Hence, text input is experimentally supported.

As of writting this, VTK SDL2 interactor does not yield the platform scan code. It simply returns the first character.
Although, [this](https://gitlab.kitware.com/vtk/vtk/-/merge_requests/8561) merge request unlocks all keycodes in vtk's SDL2 interactor.

The idea of this project was to give VTK control over the rendering of ImGUI elements in addition to event management.
VTK's event management is limited to basics on all platforms. With such fragile support, I honestly think its not a
good idea to let VTK manage events yet. For now, https://github.com/trlsmax/imgui-vtk has a better look and feel, 
if you prefer managing the event loop.