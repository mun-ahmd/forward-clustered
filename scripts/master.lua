local rs = rendering
local sc = scene

-- Initial load of the rendering and scene scripts.
dofile("scripts/rendering.lua")
dofile("scripts/scene/main.lua")

-- Rendering hot-reload: destroys LUA-tagged GPU resources only, scene objects intact.
rs.setHotReloadCallback(function()
    rs.log("[master] Hot-reloading rendering.lua ...")
    rs.destroyAllLuaResources()
    dofile("scripts/rendering.lua")
    rs.log("[master] Rendering hot-reload complete.")
end)

-- Scene reload: destroys SCENE-tagged GPU resources and scene objects, rendering intact.
sc.setSceneReloadCallback(function()
    rs.log("[master] Reloading scene ...")
    sc.destroyAllSceneResources()
    dofile("scripts/scene/main.lua")
    rs.log("[master] Scene reload complete.")
end)
