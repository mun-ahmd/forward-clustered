# CLAUDE.md — guide for AI agents (forward-clustered / Renderer)

This document summarizes what matters when reading, building, or changing this codebase.

## What this project is

- **C++17** desktop **Vulkan** renderer: clustered forward lighting, GLTF-style scenes, ImGui, runtime GLSL compilation via **Shaderc**, GPU memory via **VMA** (`vk_mem_alloc.h`).
- **Executable name:** `Renderer` (CMake target and historical Visual Studio name).
- **Entry point:** `src/main.cpp` → `Application::run()` (large file; core init, frame loop, cluster lighting compute, etc.).
- **Surface / windowing:** **GLFW** + `glfwCreateWindowSurface` in `src/core/core.hpp` (portable; not Win32-only surface code).

## Repository layout

```
src/
├── main.cpp                   App entry, Vulkan init, frame loop, cluster compute dispatch
├── core/                      Vulkan foundation — no local deps except each other
│   ├── core.hpp               VulkanCore: instance, device, queues, command pools; shader compile
│   ├── vulkan_utils.hpp       Pipeline/renderpass/shader helpers
│   ├── buffer.hpp             VMA-backed GPU buffer (mapped pointers can move — see buffer.hpp comments)
│   ├── buffer_helpers.hpp     Descriptor binding; SSBO/IAResource helpers
│   ├── image.hpp              GPU image + image view
│   ├── swapchain.hpp          Swapchain lifecycle
│   ├── frame.hpp              Per-frame sync primitives
│   └── window_helper.h        GLFW window creation
├── assets/                    Loaders and runtime asset types
│   ├── MeshLoader.hpp/.cpp    cgltf (GLTF/GLB) + fast_obj (OBJ) loading
│   ├── ImageLoader.hpp/.cpp   stb image load/resize
│   ├── asyncImageLoader.hpp   Concurrent texture upload (uses ConcurrentQueue)
│   ├── ConcurrentQueue.hpp    Thread-safe queue
│   ├── GLTFScene.hpp          Loaded scene: meshes, materials, lights
│   ├── mesh.hpp               Mesh struct (vertices, indices, VMA buffers)
│   ├── material.hpp           PBR material + GPU descriptor state
│   └── light.hpp              Point/directional light + GPU buffer
├── renderer/                  Render passes and top-level orchestration
│   ├── rendering.hpp          Owns all passes; main rendering entry point
│   ├── forwardRenderer.hpp    Clustered forward lighting pass
│   ├── depthOnlyRenderer.hpp  Depth pre-pass
│   ├── postProcessing.hpp     Fullscreen post-process pass
│   ├── skybox.hpp             Cubemap skybox
│   └── rendererBase.hpp       Shared base class
├── scripting/                 LuaJIT / sol2 resource server
│   ├── RenderingServer.hpp/.cpp  sol2 bindings for create-info types and resource APIs
│   ├── RenderingCreateInfo.hpp   Vulkan-style create structs exposed to Lua
│   ├── RenderingResources.hpp    GPU resource containers
│   ├── EnumsFromStrings.hpp      Generated Vulkan enum ↔ string tables
│   └── EnumStringConversionBase.hpp  Base for enum map utilities
└── util/
    ├── cameraObj.h/.cpp        Camera transform and input
    ├── imgui_helper.hpp        Dear ImGui integration
    └── storage_helper.hpp      Structured storage layout helpers

vendor/                        Vendored single-file C libs (compiled as own translation units)
    fast_obj.h / fast_obj.c

Include/                       Vendored headers — do NOT reorganize
    glm/, sol/, vulkan/, shaderc/, stb_image*, vk_mem_alloc.h, …
Imgui/                         Dear ImGui source + GLFW/Vulkan backends
Shaders/                       GLSL sources; compiled to SPIR-V at runtime
lua/                           LuaJIT headers; link against system libluajit-5.1
enumStringMapGenerationScript/ Python script + enum text files that produce EnumsFromStrings.hpp
```

### Include path convention

`src/` is on the compiler include path. All project includes use layer-prefixed paths:

```cpp
#include "core/buffer.hpp"        // from anywhere
#include "assets/GLTFScene.hpp"
#include "renderer/rendering.hpp"
#include "scripting/RenderingServer.hpp"
#include "util/imgui_helper.hpp"
```

Within a layer, files may also include their siblings with the same prefix (e.g., `src/core/swapchain.hpp` includes `"core/image.hpp"`). Vendor headers (`Include/`, `lua/`, `Imgui/`) are found via their own include directories and use angle-bracket or bare-name includes unchanged.

## Build system (do this first)

- **Primary:** root `CMakeLists.txt` (minimum CMake 3.16).
- **C++ standard:** enforced as **ISO C++17** on the `Renderer` target (`CMAKE_CXX_EXTENSIONS OFF`, `cxx_std_17`).
- **cgltf:** not vendored in-tree; **FetchContent** pulls jkuhlmann/cgltf at configure time and stages `cgltf.h` under `build/generated_includes/cgltf/` so `#include "cgltf/cgltf.h"` in `MeshLoader.cpp` works. **Network required** on first configure unless cached.
- **Compile commands:** `CMAKE_EXPORT_COMPILE_COMMANDS ON` (useful for clangd).

### Linux dependencies (summary)

Install dev packages for **Vulkan**, **GLFW3**, **shaderc** (shared), **LuaJIT**, plus **cmake**, **ninja**, **pkg-config**. Exact commands: **`BUILD_LINUX.md`**.

- **Shaderc:** link the shared library; **headers** under `Include/shaderc/` are vendored.
- **LuaJIT:** use repo `lua/` for includes; **do not** mix in another Lua's headers.

### Compiler choice

- **GCC** (`g++`) or **Clang** (`clang++`): set on **first** `cmake` configure, e.g. `-DCMAKE_CXX_COMPILER=clang++`, or use a separate build directory per toolchain. See **`BUILD_LINUX.md`**.

### Legacy Windows

- `VulkanRenderer.vcxproj` documents older MSVC/ClangCL settings and `Libs/` linking; **`Libs/` may be empty** in git — prefer CMake + SDK/vcpkg-style layout on Windows if you extend support.

## Running the binary

- Output: `build/Renderer` (single-config generators).
- **Working directory matters:** code loads paths like `Shaders/...` and `shaders/clusters.comp` (note **mixed case**). On Linux the filesystem is **case-sensitive**; wrong casing fails silently at file open.
- **Shaders/assets** may not be in the repo snapshot; confirm paths relative to **cwd** when debugging missing-file errors.

## Vulkan / GPU assumptions

- **API version:** `VK_API_VERSION_1_3` (`APP_VK_API_VERSION` in `src/core/core.hpp`).
- **Validation layers:** enabled when **`NDEBUG` is not defined** (Debug builds). Release/RelWithDebInfo with `NDEBUG` disables layers.
- Device features checked in `VulkanCore` include **descriptor indexing**, **dynamic rendering**, **sampler anisotropy**, etc. — changing minimum GPU requirements needs coordinated changes there and in shaders.

## Scripting layer (easy to break)

- **`src/scripting/RenderingServer.hpp`** includes `<sol/sol.hpp>` and `<lua.hpp>`; `RenderingServer.cpp` registers many **`sol::state_view::new_usertype<>`** bindings for `Rendering::*CreateInfo` structs and server APIs.
- **`src/scripting/RenderingCreateInfo.hpp`:** `EXPORTPROP` / `EXPORTCLASS` macros are currently **no-ops** but structure the file for possible tooling; keep them consistent if you edit patterns.
- **Do not** use **double-underscore** identifiers (e.g. `__TYPEID__`) in new code — **reserved**; GCC/Clang will not behave like MSVC.

## Portability lessons already learned (avoid regressions)

1. **`std::exception`:** standard `std::exception` has **no `const char*` constructor**. Use **`std::runtime_error`** (or subclass) for messages (`src/util/imgui_helper.hpp` and elsewhere).
2. **`Buffer`:** `allocattedInfo` is **private**; use **`Buffer::getMappedData()`** instead of touching VMA allocation info from outside (`src/core/buffer_helpers.hpp`).
3. **sol2 + GCC/Clang:** vendored **`Include/sol/function_types_stateless.hpp`** has two patches:
   - `upvalue_this_member_variable::call` uses **`noexcept(false)`** (instead of the original `noexcept(std::is_nothrow_copy_assignable_v<T>)`) so Clang can form `lua_CFunction` pointers.
   - `operator()` calls **`call<false, false>(L)`** (instead of bare `call(L)`) so GCC can resolve the two-parameter template without deduction failure.
   If you **upgrade sol2**, re-check this file or upstream release notes for equivalent fixes.
4. **MSVC-only "permissive" patterns:** prefer standards-conforming C++; validate with **GCC or Clang** if possible.

## Style and change discipline (project norms)

- Prefer **small, focused diffs**; match existing naming and header-only vs `.cpp` split.
- Large files (`src/main.cpp`, `src/renderer/rendering.hpp`) drive much behavior — grep before assuming a type lives in an obvious file.
- **GPU / VMA:** comments in `src/core/buffer.hpp` warn that mapped pointers can change after defragmentation; treat long-lived raw pointers as suspect.
- **Include paths:** new files must use the `"layer/file.hpp"` prefix convention — do not add bare-name includes for project files.

## Tests and automation

- No dedicated **unit test** harness is wired in CMake. Verification is **build + run** on target GPU.

## Optional / peripheral

- **`ImageResizer/`** subtree exists; the main **Renderer** CMake graph does not depend on it.
- **`src/mainRandom.cpp`** is entirely commented out — kept as an experimental scratch file.
- **`src/renderer/screenSpaceAO.hpp`** and **`src/renderer/sdfGenerator.hpp`** are stubs for future work.

## Quick reference commands

```bash
# Configure + build (Linux)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run (from repo root so Shaders/ paths resolve)
./build/Renderer
```

When suggesting edits, prefer **CMake** as the source of truth for sources and flags, and point humans to **`BUILD_LINUX.md`** for environment setup.
