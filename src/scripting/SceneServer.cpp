#include "scripting/SceneServer.hpp"
#include "scripting/RenderingServer.hpp"

#include <iostream>

void SceneServer::init(RenderingServer& rs) {
	renderingServer = &rs;
	registerSceneBindings();
}

void SceneServer::registerSceneBindings() {
	sol::state_view state = renderingServer->getLuaState();

	// Bind CreateInfo types so Lua can construct them
	state.new_usertype<SceneMeshCreateInfo>(
		"SceneMeshCreateInfo",
		sol::constructors<SceneMeshCreateInfo()>(),
		"vertexBuffer", &SceneMeshCreateInfo::vertexBuffer,
		"indexBuffer",  &SceneMeshCreateInfo::indexBuffer,
		"indexCount",   &SceneMeshCreateInfo::indexCount,
		"indexType",    &SceneMeshCreateInfo::indexType
	);
	state.new_usertype<SceneLightCreateInfo>(
		"SceneLightCreateInfo",
		sol::constructors<SceneLightCreateInfo()>(),
		"type",      &SceneLightCreateInfo::type,
		"posX",      &SceneLightCreateInfo::posX,
		"posY",      &SceneLightCreateInfo::posY,
		"posZ",      &SceneLightCreateInfo::posZ,
		"colR",      &SceneLightCreateInfo::colR,
		"colG",      &SceneLightCreateInfo::colG,
		"colB",      &SceneLightCreateInfo::colB,
		"intensity", &SceneLightCreateInfo::intensity
	);
	state.new_usertype<SceneLightProperties>(
		"SceneLightProperties",
		sol::constructors<SceneLightProperties()>(),
		"type",      &SceneLightProperties::type,
		"posX",      &SceneLightProperties::posX,
		"posY",      &SceneLightProperties::posY,
		"posZ",      &SceneLightProperties::posZ,
		"colR",      &SceneLightProperties::colR,
		"colG",      &SceneLightProperties::colG,
		"colB",      &SceneLightProperties::colB,
		"intensity", &SceneLightProperties::intensity
	);
	state.new_usertype<SceneMaterialCreateInfo>(
		"SceneMaterialCreateInfo",
		sol::constructors<SceneMaterialCreateInfo()>(),
		"descriptorSet", &SceneMaterialCreateInfo::descriptorSet,
		"dynamicOffset", &SceneMaterialCreateInfo::dynamicOffset
	);
	state.new_usertype<SceneInstanceCreateInfo>(
		"SceneInstanceCreateInfo",
		sol::constructors<SceneInstanceCreateInfo()>(),
		"mesh",      &SceneInstanceCreateInfo::mesh,
		"material",  &SceneInstanceCreateInfo::material,
		"transform", &SceneInstanceCreateInfo::transform
	);
	state.new_usertype<SceneInstanceProperties>(
		"SceneInstanceProperties",
		sol::constructors<SceneInstanceProperties()>(),
		"transform", &SceneInstanceProperties::transform
	);

	sol::table scene = state.create_named_table("scene");
	state["sc"] = scene;

	SceneServer* server = this;
	scene.set_function("setFrameCallback",        &SceneServer::setFrameCallback,        server);
	scene.set_function("setSceneReloadCallback",  &SceneServer::setSceneReloadCallback,  server);
	scene.set_function("destroyAllSceneResources",&SceneServer::destroyAllSceneResources, server);
	scene.set_function("createMesh",              &SceneServer::createMesh,              server);
	scene.set_function("destroyMesh",             &SceneServer::destroyMesh,             server);
	scene.set_function("createLight",             &SceneServer::createLight,             server);
	scene.set_function("updateLight",             &SceneServer::updateLight,             server);
	scene.set_function("destroyLight",            &SceneServer::destroyLight,            server);
	scene.set_function("createMaterial",          &SceneServer::createMaterial,          server);
	scene.set_function("destroyMaterial",         &SceneServer::destroyMaterial,         server);
	scene.set_function("createInstance",          &SceneServer::createInstance,          server);
	scene.set_function("updateInstance",          &SceneServer::updateInstance,          server);
	scene.set_function("destroyInstance",         &SceneServer::destroyInstance,         server);
	scene.set_function("getDrawables",            &SceneServer::getDrawables,            server);
}

void SceneServer::callFrameCallback(uint32_t frameIndex) {
	if (frameCallback.valid()) {
		sol::protected_function_result result = frameCallback(frameIndex);
		if (!result.valid()) {
			sol::error err = result;
			std::cerr << "Error in scene frame callback: " << err.what() << std::endl;
		}
	}
}

void SceneServer::callSceneReloadCallback() {
	if (sceneReloadCallback.valid()) {
		sol::protected_function_result result = sceneReloadCallback();
		if (!result.valid()) {
			sol::error err = result;
			std::cerr << "Error in scene reload callback: " << err.what() << std::endl;
		}
	}
}

void SceneServer::setFrameCallback(sol::protected_function fn) {
	frameCallback = std::move(fn);
}

void SceneServer::setSceneReloadCallback(sol::protected_function fn) {
	sceneReloadCallback = std::move(fn);
}

void SceneServer::destroyAllSceneResources() {
	frameCallback = sol::protected_function{};

	renderingServer->setOperatingUser(RenderingServer::ResourceUser::SCENE);
	renderingServer->destroyAllSceneResources();
	renderingServer->setOperatingUser(RenderingServer::ResourceUser::NONE);

	meshStore.clear();
	lightStore.clear();
	materialStore.clear();
	instanceStore.clear();
}

SceneObjectID SceneServer::createMesh(SceneMeshCreateInfo info) {
	SceneObjectID id = nextId++;
	meshStore[id] = SceneObj::Mesh{
		info.vertexBuffer,
		info.indexBuffer,
		info.indexCount,
		info.indexType
	};
	return id;
}

void SceneServer::destroyMesh(SceneObjectID id) {
	meshStore.erase(id);
}

SceneObjectID SceneServer::createLight(SceneLightCreateInfo info) {
	SceneObjectID id = nextId++;
	lightStore[id] = SceneObj::Light{
		info.type,
		info.posX, info.posY, info.posZ,
		info.colR, info.colG, info.colB,
		info.intensity
	};
	return id;
}

void SceneServer::updateLight(SceneObjectID id, SceneLightProperties props) {
	auto it = lightStore.find(id);
	if (it == lightStore.end()) return;
	it->second = SceneObj::Light{
		props.type,
		props.posX, props.posY, props.posZ,
		props.colR, props.colG, props.colB,
		props.intensity
	};
}

void SceneServer::destroyLight(SceneObjectID id) {
	lightStore.erase(id);
}

SceneObjectID SceneServer::createMaterial(SceneMaterialCreateInfo info) {
	SceneObjectID id = nextId++;
	materialStore[id] = SceneObj::Material{ info.descriptorSet, info.dynamicOffset };
	return id;
}

void SceneServer::destroyMaterial(SceneObjectID id) {
	materialStore.erase(id);
}

SceneObjectID SceneServer::createInstance(SceneInstanceCreateInfo info) {
	SceneObjectID id = nextId++;
	SceneObj::Instance inst;
	inst.mesh     = info.mesh;
	inst.material = info.material;
	if (info.transform.valid()) {
		for (int i = 0; i < 16; ++i)
			inst.transform[i] = info.transform.get<float>(i + 1);
	}
	instanceStore[id] = inst;
	return id;
}

void SceneServer::updateInstance(SceneObjectID id, SceneInstanceProperties props) {
	auto it = instanceStore.find(id);
	if (it == instanceStore.end()) return;
	if (props.transform.valid()) {
		for (int i = 0; i < 16; ++i)
			it->second.transform[i] = props.transform.get<float>(i + 1);
	}
}

void SceneServer::destroyInstance(SceneObjectID id) {
	instanceStore.erase(id);
}

sol::table SceneServer::getDrawables() {
	sol::state_view state = renderingServer->getLuaState();
	sol::table result     = state.create_table();

	int idx = 1;
	for (auto& [instId, inst] : instanceStore) {
		auto meshIt = meshStore.find(inst.mesh);
		if (meshIt == meshStore.end()) continue;
		const SceneObj::Mesh& mesh = meshIt->second;

		sol::table entry = state.create_table();
		entry["vertexBuffer"] = mesh.vertexBuffer;
		entry["indexBuffer"]  = mesh.indexBuffer;
		entry["indexCount"]   = mesh.indexCount;
		entry["indexType"]    = mesh.indexType;

		// material dynamic offset if present
		auto matIt = materialStore.find(inst.material);
		entry["materialDynamicOffset"] = (matIt != materialStore.end()) ? matIt->second.dynamicOffset : 0u;

		// column-major transform as a 16-element table
		sol::table xform = state.create_table();
		for (int i = 0; i < 16; ++i) xform[i + 1] = inst.transform[i];
		entry["transform"] = xform;

		result[idx++] = entry;
	}
	return result;
}
