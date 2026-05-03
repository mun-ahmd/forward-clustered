local rs = rendering

-- Initial load of the rendering script.
dofile("scripts/rendering.lua")

-- Register the hot-reload handler with C++.
-- C++ calls this (after vkDeviceWaitIdle) when the user presses
-- "Rebuild Shading Pipeline" in the ImGui panel.
rs.setHotReloadCallback(function()
    print("[master] Hot-reloading rendering.lua ...")
    rs.destroyAllLuaResources()
    dofile("scripts/rendering.lua")
    print("[master] Hot-reload complete.")
end)
