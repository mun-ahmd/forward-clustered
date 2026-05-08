-- Cloud demo scene: no GLTF; only scripts/rendering/cloud_rendering.lua.

local M = {}

M.compatibleRendering = {
  "^scripts/rendering/cloud_rendering%.lua$",
}
M.defaultRendering = "scripts/rendering/cloud_rendering.lua"
M.loadsGltf = false

function M.init()
  local sc = scene
  sc.setFrameCallback(function(frameIndex)
  end)
end

return M
