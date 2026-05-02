# forward-clustered

A real-time **Vulkan 1.3** renderer written in C++17, implementing clustered forward lighting with a LuaJIT scripting layer for scene and resource configuration.

## Features

- **Clustered forward lighting** — GPU compute subdivides the view frustum into clusters; each fragment looks up only the lights that overlap its cluster
- **GLTF scene loading** — `cgltf` for binary/text GLTF, `fast_obj` for OBJ; async texture streaming via a thread-safe image loader
- **PBR materials** — base color, normal, roughness/metalness textures; descriptor indexing for bindless texture access
- **Depth pre-pass** — separate depth-only renderer reduces overdraw in the main forward pass
- **Post-processing pass** — fullscreen effects pipeline (tone-mapping, etc.)
- **Skybox / environment** — cubemap loading and rendering
- **Dear ImGui** — in-frame debug UI with GLFW + Vulkan backends
- **LuaJIT scripting** — `RenderingServer` exposes Vulkan create-info structs and resource APIs to Lua via **sol2**
- **Runtime GLSL compilation** — shaders compiled to SPIR-V at startup via **Shaderc**; no precompiled blobs required
- **VMA memory management** — all buffers and images allocated through Vulkan Memory Allocator

## Requirements

- GPU with **Vulkan 1.3** support, descriptor indexing, dynamic rendering, and sampler anisotropy
- Linux or Windows (Linux is the primary development target)
- CMake 3.16+, a C++17 compiler (GCC 10+ or Clang 12+)

## Quick start (Linux)

Install dependencies — see [BUILD_LINUX.md](BUILD_LINUX.md) for exact package names per distro.

```bash
# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run (working directory must be the repo root — shaders and assets are loaded by relative path)
./build/Renderer
```

The first configure fetches `cgltf` via CMake `FetchContent` (network required once).

## Project structure

```
src/
├── main.cpp                   App entry, Vulkan init, frame loop
├── core/                      Vulkan foundation (device, buffers, images, swapchain)
│   ├── core.hpp               VulkanCore: instance, device, queues, command pools
│   ├── vulkan_utils.hpp       Pipeline/shader/renderpass helpers
│   ├── buffer.hpp             VMA-backed GPU buffer
│   ├── buffer_helpers.hpp     Descriptor binding, SSBO helpers
│   ├── image.hpp              GPU image + image view wrapper
│   ├── swapchain.hpp          Swapchain management
│   ├── frame.hpp              Per-frame synchronization resources
│   └── window_helper.h        GLFW window creation
├── assets/                    Loaders and runtime asset types
│   ├── MeshLoader.hpp/.cpp    GLTF (cgltf) and OBJ (fast_obj) mesh loading
│   ├── ImageLoader.hpp/.cpp   STB image load/resize
│   ├── asyncImageLoader.hpp   Concurrent image upload pipeline
│   ├── ConcurrentQueue.hpp    Thread-safe queue used by async loader
│   ├── GLTFScene.hpp          Loaded scene graph (meshes, materials, lights)
│   ├── mesh.hpp               Mesh data structure (vertices, indices)
│   ├── material.hpp           PBR material properties and GPU buffers
│   └── light.hpp              Point/directional light definitions
├── renderer/                  Render passes and orchestration
│   ├── rendering.hpp          Top-level renderer (owns all passes)
│   ├── forwardRenderer.hpp    Clustered forward lighting pass
│   ├── depthOnlyRenderer.hpp  Depth pre-pass
│   ├── postProcessing.hpp     Post-process fullscreen pass
│   ├── skybox.hpp             Cubemap skybox
│   └── rendererBase.hpp       Shared base for renderer classes
├── scripting/                 LuaJIT / sol2 resource server
│   ├── RenderingServer.hpp/.cpp  sol2 bindings: create-info types, resource APIs
│   ├── RenderingCreateInfo.hpp   Vulkan-style create structs exposed to Lua
│   ├── RenderingResources.hpp    GPU resource containers
│   └── EnumsFromStrings.hpp      Vulkan enum ↔ string tables (generated)
└── util/                      Application-level helpers
    ├── cameraObj.h/.cpp        Camera transform and movement
    ├── imgui_helper.hpp        ImGui integration helpers
    └── storage_helper.hpp      Structured storage layout utilities

vendor/                        Vendored single-file C libraries
    fast_obj.h / fast_obj.c    Minimal OBJ parser

Include/                       Vendored C++ headers (glm, sol2, Vulkan, shaderc, stb, VMA…)
Imgui/                         Dear ImGui source
Shaders/                       GLSL shader sources (compiled at runtime)
lua/                           LuaJIT headers (link against system libluajit-5.1)
```

## Windows

A legacy `VulkanRenderer.vcxproj` / `.sln` is included for reference. CMake is the authoritative build system. `Libs/` may be absent in some checkouts — prefer CMake with a Vulkan SDK or vcpkg layout.
