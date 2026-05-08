local rs = rendering
local sc = scene

-- C++ sets before execute: DISCOVERED_SCENE_SCRIPTS, DISCOVERED_RENDERING_SCRIPTS (1-based arrays).
-- Default scene if nothing else applies (keep in sync with Application::pickInitialSceneScriptPath in main.cpp).
local DEFAULT_SCENE_SCRIPT = "scripts/scene/main.lua"

RENDERING_SCRIPT = nil
SCENE_SCRIPT = nil
CURRENT_SCENE_SCRIPT = nil
CURRENT_RENDERING_SCRIPT = nil
CURRENT_ALLOWED_RENDERING = {}
CURRENT_SCENE_LOADS_GLTF = true
PERSISTED_RENDERING_SCRIPT = nil

local kActiveSceneScript = "activeSceneScript"

local function storeKeyRendering(scenePath)
  return "renderingScriptFor_" .. string.gsub(scenePath, "/", "_")
end

local function loadPersistedRenderingForPath(scenePath)
  local pk = storeKeyRendering(scenePath)
  if rs.objectStoreHasKey(pk) then
    return rs.objectStoreGetString(pk)
  end
  return nil
end

--- Same rules as C++ pickInitialSceneScriptPath() so startup geometry matches this script.
local function pickInitialSceneScript()
  local disc = DISCOVERED_SCENE_SCRIPTS
  if rs.objectStoreHasKey(kActiveSceneScript) then
    local stored = rs.objectStoreGetString(kActiveSceneScript)
    if type(stored) == "string" and stored ~= "" and disc then
      for i = 1, #disc do
        if disc[i] == stored then
          return stored
        end
      end
    end
  end
  if disc then
    for i = 1, #disc do
      if disc[i] == DEFAULT_SCENE_SCRIPT then
        return DEFAULT_SCENE_SCRIPT
      end
    end
    if #disc > 0 then
      return disc[1]
    end
  end
  return DEFAULT_SCENE_SCRIPT
end

local function pathMatchesAnyPattern(path, patterns)
  if not patterns then
    return false
  end
  for _, pat in ipairs(patterns) do
    if string.find(path, pat, 1) then
      return true
    end
  end
  return false
end

local function pickRenderingPath(sceneMod)
  local persisted = PERSISTED_RENDERING_SCRIPT
  if type(persisted) == "string" and persisted ~= "" then
    if pathMatchesAnyPattern(persisted, sceneMod.compatibleRendering) then
      return persisted
    end
    rs.log("[master] Persisted rendering script is not compatible with this scene; using default.")
  end
  return sceneMod.defaultRendering
end

local function rebuildAllowedRendering(sceneMod)
  CURRENT_ALLOWED_RENDERING = {}
  local disc = DISCOVERED_RENDERING_SCRIPTS
  if not disc then
    return
  end
  for i = 1, #disc do
    local p = disc[i]
    if pathMatchesAnyPattern(p, sceneMod.compatibleRendering) then
      table.insert(CURRENT_ALLOWED_RENDERING, p)
    end
  end
end

local function loadSceneModule(path)
  local chunk = assert(loadfile(path))
  return chunk()
end

__masterResolveRenderingPathForScene = function(scenePath)
  local sm = loadSceneModule(scenePath)
  return pickRenderingPath(sm)
end

__masterReloadRenderingBody = function()
  rs.destroyAllLuaResources()
  rs.setResourceUser("lua")
  assert(loadfile(CURRENT_RENDERING_SCRIPT))()
  rs.setResourceUser("none")
end

__masterReloadSceneBody = function()
  sc.destroyAllSceneResources()
  local sm = loadSceneModule(CURRENT_SCENE_SCRIPT)
  rebuildAllowedRendering(sm)
  rs.setResourceUser("scene")
  sm.init()
  rs.setResourceUser("none")
end

function __masterApplySceneSwitch(newScenePath)
  rs.objectStoreSetString(kActiveSceneScript, newScenePath)
  CURRENT_SCENE_SCRIPT = newScenePath

  local persisted = loadPersistedRenderingForPath(newScenePath)
  PERSISTED_RENDERING_SCRIPT = persisted

  CURRENT_RENDERING_SCRIPT = __masterResolveRenderingPathForScene(newScenePath)

  __masterReloadRenderingBody()
  masterCpp.connectForwardOutputs()

  local peek = loadSceneModule(newScenePath)
  CURRENT_SCENE_LOADS_GLTF = peek.loadsGltf ~= false
  masterCpp.reloadSceneGeometry(CURRENT_SCENE_LOADS_GLTF)

  __masterReloadSceneBody()
  masterCpp.connectForwardOutputs()
  masterCpp.connectSceneToRenderer()
end

function __masterApplyRenderingSwitch(newRenderingPath)
  local pk = storeKeyRendering(CURRENT_SCENE_SCRIPT)
  rs.objectStoreSetString(pk, newRenderingPath)
  PERSISTED_RENDERING_SCRIPT = newRenderingPath
  CURRENT_RENDERING_SCRIPT = newRenderingPath

  __masterReloadRenderingBody()
  masterCpp.connectForwardOutputs()
end

local function wireCallbacks()
  rs.setHotReloadCallback(function()
    rs.log("[master] Hot-reloading " .. tostring(CURRENT_RENDERING_SCRIPT) .. " ...")
    __masterReloadRenderingBody()
    rs.log("[master] Rendering script reload complete.")
  end)

  sc.setSceneReloadCallback(function()
    rs.log("[master] Reloading scene " .. tostring(CURRENT_SCENE_SCRIPT) .. " ...")
    __masterReloadSceneBody()
    masterCpp.connectForwardOutputs()
    masterCpp.connectSceneToRenderer()
    rs.log("[master] Scene reload complete.")
  end)
end

local function masterInit()
  SCENE_SCRIPT = pickInitialSceneScript()
  CURRENT_SCENE_SCRIPT = SCENE_SCRIPT

  PERSISTED_RENDERING_SCRIPT = loadPersistedRenderingForPath(SCENE_SCRIPT)

  local sceneMod = loadSceneModule(SCENE_SCRIPT)
  rebuildAllowedRendering(sceneMod)

  RENDERING_SCRIPT = pickRenderingPath(sceneMod)
  CURRENT_RENDERING_SCRIPT = RENDERING_SCRIPT

  CURRENT_SCENE_LOADS_GLTF = sceneMod.loadsGltf ~= false

  rs.setResourceUser("lua")
  dofile(RENDERING_SCRIPT)
  rs.setResourceUser("scene")
  sceneMod.init()
  rs.setResourceUser("none")

  wireCallbacks()
end

masterInit()
