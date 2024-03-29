# vtkDearImGUIInjector
[![Deploy WebAssembly binary](https://github.com/jspanchu/vtkDearImGUIInjector/actions/workflows/build-wasm.yml/badge.svg)](https://github.com/jspanchu/vtkDearImGUIInjector/actions/workflows/build-wasm.yml)

Inject [DearImGUI](https://github.com/ocornut/imgui) into an existing VTK application.

![image](https://github.com/jspanchu/vtkDearImGUIInjector/assets/40538987/f2512679-d047-491f-a2c8-9cc7b38e8ca0)

# Build

## Desktop

Build the sample program by configuring the project with CMake. This step assumes you've built VTK. 
```bash
cmake -GNinja -S . -B out -DVTK_DIR=/path/to/vtk/install/lib/cmake/vtk
cmake --build out
```

Now you can run `./out/sample/main`.

## WebAssembly (for browsers)

Compile the C++ sources against a pre-built VTK wasm docker image using npm and itk-wasm-cli. This is the very same method
used for the [live](https://jspanchu.dev/vtkDearImGUIInjector) web demo.

```bash
npm run build-wasm
```

# Serve web page

```bash
python3 -m http.server -d build-emscripten/sample 8080
# Open browser and visit http://localhost:8080
```

# Notes
A very closely related project is phcerdan's https://github.com/phcerdan/vtkImGuiAdapter.
The two are slightly different in that:
1. With vtkDearImGUIInjector, 
    
    a. you do not have to take care of ImGUI contexts yourself.
    
    b. no need to write an event loop.
2. With vtkImGUIAdapter,
    
    a. you are responsible for ensuring the sdl2 window is setup and the ImGUI context is initialized.
    
    b. you must write an event loop in your application, ensure events are processed and ensure NewFrame/EndFrame are called.
