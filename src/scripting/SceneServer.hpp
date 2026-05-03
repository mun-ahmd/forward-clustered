#pragma once
#include <unordered_map>
#include <string>

#include <sol/sol.hpp>

#include "scripting/SceneCreateInfo.hpp"

class RenderingServer;

class SceneServer {
public:
	// Must be called after RenderingServer::registerRenderingScript() so that the
	// sol::state already exists. Registers the "scene" and "sc" globals into it.
	void init(RenderingServer& rs);

	// Called from main.cpp each frame after the rendering frame callback.
	void callFrameCallback(uint32_t frameIndex);

	// Called from main.cpp when the user presses "Reload Scene".
	// Delegates to the sceneReloadCallback set by master.lua.
	void callSceneReloadCallback();

	// --- Lua-callable API ---

	void setFrameCallback(sol::protected_function fn);
	void setSceneReloadCallback(sol::protected_function fn);

	// Destroys all SCENE-tagged Vulkan resources via RenderingServer, then
	// clears the high-level scene object stores.
	void destroyAllSceneResources();

	SceneObjectID createMesh(SceneMeshCreateInfo info);
	void          destroyMesh(SceneObjectID id);

	SceneObjectID createLight(SceneLightCreateInfo info);
	void          updateLight(SceneObjectID id, SceneLightProperties props);
	void          destroyLight(SceneObjectID id);

	SceneObjectID createMaterial(SceneMaterialCreateInfo info);
	void          destroyMaterial(SceneObjectID id);

	SceneObjectID createInstance(SceneInstanceCreateInfo info);
	void          updateInstance(SceneObjectID id, SceneInstanceProperties props);
	void          destroyInstance(SceneObjectID id);

	// Returns a Lua array of tables compatible with rendering.getSceneDrawables() format.
	sol::table getDrawables();

private:
	void registerSceneBindings();

	RenderingServer*         renderingServer = nullptr;
	sol::protected_function  frameCallback;
	sol::protected_function  sceneReloadCallback;

	uint32_t nextId = 1;
	std::unordered_map<SceneObjectID, SceneObj::Mesh>     meshStore;
	std::unordered_map<SceneObjectID, SceneObj::Light>    lightStore;
	std::unordered_map<SceneObjectID, SceneObj::Material> materialStore;
	std::unordered_map<SceneObjectID, SceneObj::Instance> instanceStore;
};
