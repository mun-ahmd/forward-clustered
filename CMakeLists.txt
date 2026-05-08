# Renderer — CMake build (Linux primary; can be used from MSVC/VS with a kit too)
#
# Quick start (see BUILD_LINUX.md for packages):
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   Optional Clang: -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
#   cmake --build build
#   cd /path/to/repo && ./build/Renderer   # cwd should contain Shaders/ and assets

cmake_minimum_required(VERSION 3.16)
project(Renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
	cgltf
	GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
	GIT_TAG v1.13
)
FetchContent_MakeAvailable(cgltf)
# MeshLoader.cpp includes "cgltf/cgltf.h"; upstream ships a single cgltf.h at repo root.
set(CGLTF_INCLUDE_STAGING "${CMAKE_BINARY_DIR}/generated_includes")
file(MAKE_DIRECTORY "${CGLTF_INCLUDE_STAGING}/cgltf")
file(COPY "${cgltf_SOURCE_DIR}/cgltf.h" DESTINATION "${CGLTF_INCLUDE_STAGING}/cgltf")

add_executable(Renderer
	src/util/cameraObj.cpp
	vendor/fast_obj.c
	src/scripting/RenderingServer.cpp
	src/scripting/RenderingServerSwapchain.cpp
	src/scripting/SceneServer.cpp
	src/assets/ImageLoader.cpp
	Imgui/imgui.cpp
	Imgui/imgui_demo.cpp
	Imgui/imgui_draw.cpp
	Imgui/imgui_impl_glfw.cpp
	Imgui/imgui_impl_vulkan.cpp
	Imgui/imgui_tables.cpp
	Imgui/imgui_widgets.cpp
	src/assets/MeshLoader.cpp
	src/main.cpp
)

set_target_properties(Renderer PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_compile_features(Renderer PRIVATE cxx_std_17)

target_include_directories(Renderer PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor"
	"${CMAKE_CURRENT_SOURCE_DIR}/Include"
	"${CMAKE_CURRENT_SOURCE_DIR}/lua"
	"${CMAKE_CURRENT_SOURCE_DIR}/Imgui"
	"${CGLTF_INCLUDE_STAGING}"
)

target_compile_definitions(Renderer PRIVATE
	"$<$<CONFIG:Release>:NDEBUG>"
	"$<$<BOOL:${MSVC}>:_CRT_SECURE_NO_WARNINGS>"
)

find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)
find_package(PkgConfig QUIET)

set(_RENDERER_GLFW_TARGET "")
find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
	set(_RENDERER_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw3)
	set(_RENDERER_GLFW_TARGET glfw3::glfw3)
endif()
if(NOT _RENDERER_GLFW_TARGET AND PkgConfig_FOUND)
	pkg_check_modules(GLFW3_PC QUIET glfw3 IMPORTED_TARGET)
	if(TARGET PkgConfig::GLFW3_PC)
		set(_RENDERER_GLFW_TARGET PkgConfig::GLFW3_PC)
	endif()
endif()
if(NOT _RENDERER_GLFW_TARGET)
	message(FATAL_ERROR
		"GLFW not found. Install libglfw3-dev (Debian/Ubuntu) or glfw-devel (Fedora), "
		"or install a GLFW that ships glfw3Config.cmake."
	)
endif()

# Shaderc: vendored headers under Include/shaderc; link shared library from SDK / distro.
if(WIN32)
	target_link_libraries(Renderer PRIVATE shaderc_shared)
else()
	set(_RENDERER_SHADERC_LINKED FALSE)
	if(PkgConfig_FOUND)
		pkg_check_modules(SHADERC shaderc)
		if(SHADERC_FOUND)
			target_link_libraries(Renderer PRIVATE ${SHADERC_LIBRARIES})
			set(_RENDERER_SHADERC_LINKED TRUE)
		endif()
	endif()
	if(NOT _RENDERER_SHADERC_LINKED)
		find_library(SHADERC_SHARED_LIB NAMES shaderc_shared shaderc
			PATHS
				/usr/lib
				/usr/local/lib
				/opt/homebrew/lib
			PATH_SUFFIXES x86_64-linux-gnu aarch64-linux-gnu
		)
		if(NOT SHADERC_SHARED_LIB)
			message(FATAL_ERROR
				"shaderc not found. Install libshaderc-dev (Debian/Ubuntu) or shaderc-devel (Fedora), "
				"or set SHADERC_SHARED_LIB to the full path of libshaderc_shared.so."
			)
		endif()
		target_link_libraries(Renderer PRIVATE "${SHADERC_SHARED_LIB}")
	endif()
endif()

# LuaJIT: use headers from this repo (lua/). Link only the system LuaJIT shared lib.
if(WIN32)
	# Match VulkanRenderer.vcxproj x64: LuaJIT import lib often named luajit.lib / lua51.lib
	find_library(LUAJIT_LIB NAMES luajit lua51 libluajit PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Libs")
	if(LUAJIT_LIB)
		target_link_libraries(Renderer PRIVATE "${LUAJIT_LIB}")
	else()
		target_link_libraries(Renderer PRIVATE luajit)
	endif()
else()
	find_library(LUAJIT_LIB NAMES luajit-5.1 luajit libluajit-5.1.so.2
		PATHS /usr/lib /usr/local/lib /opt/homebrew/lib
		PATH_SUFFIXES x86_64-linux-gnu aarch64-linux-gnu
	)
	if(NOT LUAJIT_LIB)
		message(FATAL_ERROR
			"LuaJIT not found. Install libluajit-5.1-dev (Debian/Ubuntu) or luajit-devel (Fedora)."
		)
	endif()
	target_link_libraries(Renderer PRIVATE "${LUAJIT_LIB}")
endif()

target_link_libraries(Renderer PRIVATE
	Vulkan::Vulkan
	"${_RENDERER_GLFW_TARGET}"
	Threads::Threads
)

if(UNIX AND NOT APPLE)
	target_link_libraries(Renderer PRIVATE ${CMAKE_DL_LIBS})
endif()
