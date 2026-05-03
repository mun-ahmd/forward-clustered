local sc = scene
local rs = rendering

-- Scene scripts run with SCENE resource tag active.
-- GPU resources (buffers, images) created via 'rendering.*' here are tagged SCENE
-- and are destroyed independently from the rendering script by sc.destroyAllSceneResources().

-- Example: register a mesh from existing rendering buffer IDs
-- local meshId = sc.createMesh({
--     vertexBuffer = vboId,
--     indexBuffer  = iboId,
--     indexCount   = 1234,
--     indexType    = "uint32",
-- })

-- Example: create a point light
-- local lightId = sc.createLight({
--     type = "point",
--     posX = 0, posY = 2, posZ = 0,
--     colR = 1, colG = 0.8, colB = 0.6,
--     intensity = 10.0,
-- })

sc.setFrameCallback(function(frameIndex)
    -- Per-frame scene updates: animate lights, update instance transforms, etc.
end)
