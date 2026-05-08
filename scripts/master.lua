local rs = rendering
local sc = scene

RENDERING_SCRIPT = "scripts/cloud_rendering.lua"
-- for normal rendering, use:
-- RENDERING_SCRIPT = "scripts/rendering.lua"

SCENE_SCRIPT = "scripts/scene/main.lua"

-- Initial load of the rendering and scene scripts.
dofile(RENDERING_SCRIPT)
dofile(SCENE_SCRIPT)

-- Rendering hot-reload: destroys LUA-tagged GPU resources only, scene objects intact.
rs.setHotReloadCallback(function()
    rs.log("[master] Hot-reloading " .. RENDERING_SCRIPT .. " ...")
    rs.destroyAllLuaResources()
    dofile(RENDERING_SCRIPT)
    rs.log("[master] Rendering script reload complete.")
end)

-- Scene reload: destroys SCENE-tagged GPU resources and scene objects, rendering intact.
sc.setSceneReloadCallback(function()
    rs.log("[master] Reloading scene ...")
    sc.destroyAllSceneResources()
    dofile(SCENE_SCRIPT)
    rs.log("[master] Scene reload complete.")
end)
