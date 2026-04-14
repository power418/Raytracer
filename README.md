<div align="center">
<h1>D3D12Engine</h1>
<div>
    <a href="https://github.com/power418/Raytracer/actions/workflows/cmake-multi-platform.yml">
      <img src="https://img.shields.io/github/actions/workflow/status/power418/Raytracer/cmake-multi-platform.yml?branch=main&style=flat-square&logo=windows&label=windows" alt="windows-ci" />
    </a>
    <a href="https://github.com/power418/Raytracer/releases">
      <img src="https://img.shields.io/github/release/power418/Raytracer.svg?style=flat-square" alt="Github All Releases" />
    </a>
</div>

<b>A minimalist DirectX 11/12 raytracing engine built to explore computational graphics, striving for an Unreal-like architecture with Unity-like lightweight simplicity.</b><br/>

<i>Because why download gigabytes of engine when you can just code your own pixels?</i><br/>
</div>

## Features

- **DirectX Backend**: Harness the power of modern graphics APIs (DirectX 12/DirectX 11).
- **Compute Shader Raytracing**: Real-time raytracing utilizing `raytrace_cs.hlsl`.
- **Post-Processing**: Integrated anti-aliasing techniques such as FXAA and SMAA.
- **Minimalist Architecture**: Lightweight and modular, akin to Unity, but providing an Unreal-esque logic workflow.
- **Interactive Environment**: Built-in camera, collision tracking, object selection, and endless grid visual systems.
- **Math Library**: Embedded lightweight utility math (Vectors, Matrices, Transforms).

## Quick Start

### Prerequisites
- Visual Studio with **C++ desktop development** workload.
- **Windows SDK** (Required for compiling DirectX shaders and headers).
- **CMake 3.15** or newer.

### Installation
```sh
git clone https://github.com/power418/Raytracer.git D3D12Engine
cd D3D12Engine
cmake -B build
cmake --build build --config Debug
```

### Basic Usage
After building the project, you run the engine from the generated executable. A debug console and window will open:

```pwsh
.\build\Debug\D3D12Engine.exe
```

*Note: Required HLSL shaders are automatically copied to the output directory alongside the executable during build by CMake.*

## Project Structure

```
D3D12Engine/
├── build/                 # Build output directory
├── include/               # Header files (Camera, GraphicsAdapter, RayTracer, etc.)
├── res/                   # Resources containing all HLSL shaders
├── src/                   # Source files and core engine logic
├── utils/                 # Math, Input, and Helper utilities
└── CMakeLists.txt         # Main CMake configure
```

## Contributing

Contributions are welcome! Submit pull requests, report issues, or suggest new features to help push the limits of this minimalist engine.

## License

Feel free to use, modify, and distribute according to the repository's license.

---

*Remember: Trust the math, respect the GPU.*