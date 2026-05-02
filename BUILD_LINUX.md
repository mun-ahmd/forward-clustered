# Building Renderer on Linux

## Dependencies

**Debian / Ubuntu**

```bash
sudo apt install \
  build-essential ninja-build cmake pkg-config \
  libvulkan-dev vulkan-validationlayers \
  libglfw3-dev \
  libshaderc-dev \
  libluajit-5.1-dev
```

Optional: install **Clang** if you want to compile with `clang++` instead of `g++`:

```bash
sudo apt install clang
```

Clang on Linux still uses the system **libstdc++** by default (same as GCC), so keep `build-essential` for that standard library.

**Fedora**

```bash
sudo dnf install \
  gcc-c++ cmake ninja-build pkgconf-pkg-config \
  vulkan-devel vulkan-validation-layers-devel \
  glfw-devel \
  shaderc-devel \
  luajit-devel
```

Optional Clang: `sudo dnf install clang`.

Install a Vulkan-capable driver and the [LunarG Vulkan SDK](https://vulkan.lunarg.com/) if you want newer layers/tools; distro packages are enough to compile and run in many cases.

## Configure and build

From the repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Using Make instead of Ninja:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Using Clang (`clang++`) instead of GCC

Pick the compiler **on the first `cmake` configure** (or use a fresh build directory after switching). Examples:

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build
```

Equivalent using environment variables for that one command:

```bash
rm -rf build
CC=clang CXX=clang++ cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The binary is `build/Renderer` (or `build/Release/Renderer` with multi-config generators).

## Running

The app loads shaders and assets with paths such as `Shaders/...` and `shaders/clusters.comp`. On Linux paths are case-sensitive: keep directory names consistent with the code.

Run with the **current working directory** set to the repo root (or wherever those folders exist), for example:

```bash
cd /path/to/forward-clustered && ./build/Renderer
```

`cgltf` is fetched automatically by CMake into the build directory the first time you configure.
