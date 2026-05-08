-- GLTF-oriented scene: only scripts/rendering.lua (stub → clustered forward implementation).

local M = {}

M.compatibleRendering = {
  "^scripts/rendering%.lua$",
}
M.defaultRendering = "scripts/rendering.lua"
M.loadsGltf = true

function M.init()
  local sc = scene
  sc.setFrameCallback(function(frameIndex)
    -- Per-frame scene updates: animate lights, update instance transforms, etc.
  end)
end

return M
