local rs = rendering

local w = rs.getSwapchainWidth()
local h = rs.getSwapchainHeight()
local depthFmt = rs.getDepthFormat()

-- ---- Forward output images ------------------------------------------------
local colorCI = ImageCreateInfo.new()
colorCI.format = "r16g16b16a16Sfloat"
colorCI.isColorAttachment = true
colorCI.isSampled = true
colorCI.numDimensions = 2
colorCI.width = w; colorCI.height = h; colorCI.depth = 1
colorCI.mipLevels = 1; colorCI.arrayLayers = 1
local colorImg = rs.createImage(colorCI)

local colorViewCI = ImageViewCreateInfo.new()
colorViewCI.aspectMask = "color"
colorViewCI.levelCount = 1; colorViewCI.arrayLayerCount = 1
local colorView = rs.createImageView(colorImg, colorViewCI)

local depthCI = ImageCreateInfo.new()
depthCI.format = depthFmt
depthCI.isDepthAttachment = true
depthCI.isSampled = true
depthCI.numDimensions = 2
depthCI.width = w; depthCI.height = h; depthCI.depth = 1
depthCI.mipLevels = 1; depthCI.arrayLayers = 1
local depthImg = rs.createImage(depthCI)

local depthViewCI = ImageViewCreateInfo.new()
depthViewCI.aspectMask = "depth"
depthViewCI.levelCount = 1; depthViewCI.arrayLayerCount = 1
local depthView = rs.createImageView(depthImg, depthViewCI)

local normalCI = ImageCreateInfo.new()
normalCI.format = "a2r10g10b10UnormPack32"
normalCI.isColorAttachment = true
normalCI.isSampled = true
normalCI.numDimensions = 2
normalCI.width = w; normalCI.height = h; normalCI.depth = 1
normalCI.mipLevels = 1; normalCI.arrayLayers = 1
local normalImg = rs.createImage(normalCI)

local normalViewCI = ImageViewCreateInfo.new()
normalViewCI.aspectMask = "color"
normalViewCI.levelCount = 1; normalViewCI.arrayLayerCount = 1
local normalView = rs.createImageView(normalImg, normalViewCI)

-- ---- Cluster compute pipeline ---------------------------------------------
local clusterShader = rs.createShaderModule("Shaders/clusters.comp", "compute")

local clusterPlCI = PipelineLayoutCreateInfo.new()
clusterPlCI:addSetLayout(rs.getGlobalDescriptorSet(0))
clusterPlCI:addSetLayout(rs.getLightsDescriptorSet(0))
clusterPlCI:addSetLayout(rs.getClusterDescriptorSet(0))
clusterPlCI:addPushConstant(0, 16, "compute")
local clusterPL = rs.createPipelineLayout(clusterPlCI)

local clusterCompCI = ComputePipelineCreateInfo.new()
clusterCompCI.computeShaderModule = clusterShader
clusterCompCI.pipelineLayout = clusterPL
local clusterPipe = rs.createComputePipeline(clusterCompCI)
rs.destroyShaderModule(clusterShader)

-- ---- Depth pre-pass pipeline (depth-only, no color attachments) -----------
local preVert = rs.createShaderModule("Shaders/prePass.vert", "vertex")
local preFrag = rs.createShaderModule("Shaders/prePass.frag", "fragment")

local prePlCI = PipelineLayoutCreateInfo.new()
prePlCI:addSetLayout(rs.getGlobalDescriptorSet(0))
prePlCI:addPushConstant(0, 64, "vertex")
local prePL = rs.createPipelineLayout(prePlCI)

local prePipeCI = PipelineCreateInfo.new()
prePipeCI.vertexShaderModule   = preVert
prePipeCI.fragmentShaderModule = preFrag
prePipeCI.pipelineLayout       = prePL
prePipeCI.sampleCount          = 1
prePipeCI.sampleShadingEnable  = false
prePipeCI.depthTestEnable      = true
prePipeCI.depthWriteEnable     = true
prePipeCI.depthBoundsTestEnable = false
prePipeCI.stencilTestEnable    = false
prePipeCI.depthCompareOp       = "less"
prePipeCI.depthAttachmentFormat = depthFmt
prePipeCI.polygonMode          = "fill"
-- Vertex3: binding 0, stride 32, pos(vec3,off=0), norm(vec3,off=12), uv(vec2,off=24)
prePipeCI:addVertexBinding(0, 32, false)
prePipeCI:addVertexAttribute(0, 0,  0, "r32g32b32Sfloat")
prePipeCI:addVertexAttribute(0, 1, 12, "r32g32b32Sfloat")
prePipeCI:addVertexAttribute(0, 2, 24, "r32g32Sfloat")
local prePipe = rs.createPipeline(prePipeCI)
rs.destroyShaderModule(preVert)
rs.destroyShaderModule(preFrag)

-- ---- Forward pass pipeline (2 color + depth, with vertex input) -----------
local fwdVert = rs.createShaderModule("Shaders/triangle.vert", "vertex")
local fwdFrag = rs.createShaderModule("Shaders/triangle.frag", "fragment")

local fwdPlCI = PipelineLayoutCreateInfo.new()
fwdPlCI:addSetLayout(rs.getGlobalDescriptorSet(0))
fwdPlCI:addSetLayout(rs.getLightsDescriptorSet(0))
fwdPlCI:addSetLayout(rs.getMaterialsDescriptorSet())
fwdPlCI:addPushConstant(0, 64, "vertex")
local fwdPL = rs.createPipelineLayout(fwdPlCI)

local fwdPipeCI = PipelineCreateInfo.new()
fwdPipeCI.vertexShaderModule   = fwdVert
fwdPipeCI.fragmentShaderModule = fwdFrag
fwdPipeCI.pipelineLayout       = fwdPL
fwdPipeCI.sampleCount          = 1
fwdPipeCI.sampleShadingEnable  = false
fwdPipeCI.depthTestEnable      = true
fwdPipeCI.depthWriteEnable     = false
fwdPipeCI.depthBoundsTestEnable = false
fwdPipeCI.stencilTestEnable    = false
fwdPipeCI.depthCompareOp       = "lessOrEqual"
fwdPipeCI.depthAttachmentFormat = depthFmt
fwdPipeCI.polygonMode          = "fill"
fwdPipeCI:addColorAttachment("r16g16b16a16Sfloat")
fwdPipeCI:addColorAttachment("a2r10g10b10UnormPack32")
fwdPipeCI:addVertexBinding(0, 32, false)
fwdPipeCI:addVertexAttribute(0, 0,  0, "r32g32b32Sfloat")
fwdPipeCI:addVertexAttribute(0, 1, 12, "r32g32b32Sfloat")
fwdPipeCI:addVertexAttribute(0, 2, 24, "r32g32Sfloat")
local fwdPipe = rs.createPipeline(fwdPipeCI)
rs.destroyShaderModule(fwdVert)
rs.destroyShaderModule(fwdFrag)

-- ---- Post-process resources (reads from Lua-owned forward images) ----------
local sampCI = SamplerCreateInfo.new()
sampCI:setMinFilter("nearest")
sampCI:setMagFilter("nearest")
local sampler = rs.createSampler(sampCI)

local poolCI = DescriptorPoolCreateInfo.new()
poolCI.maxSets = 1
poolCI.combinedImageSamplerCount = 3
local pool = rs.createDescriptorPool(poolCI)

local dsCI = DescriptorSetCreateInfo.new()
dsCI.descriptorPool = pool
dsCI:addBinding(0, 1, "combinedImageSampler", "fragment", false)
dsCI:addBinding(1, 1, "combinedImageSampler", "fragment", false)
dsCI:addBinding(2, 1, "combinedImageSampler", "fragment", false)
local ppDS = rs.createDescriptorSet(dsCI)

rs.writeCombinedImageSamplerToDescriptorSet(ppDS, 0, 0, colorView,  sampler, "shaderReadOnlyOptimal")
rs.writeCombinedImageSamplerToDescriptorSet(ppDS, 1, 0, depthView,  sampler, "shaderReadOnlyOptimal")
rs.writeCombinedImageSamplerToDescriptorSet(ppDS, 2, 0, normalView, sampler, "shaderReadOnlyOptimal")

local ppImgCI = ImageCreateInfo.new()
ppImgCI.format = "r8g8b8a8Unorm"
ppImgCI.isColorAttachment = true
ppImgCI.isSampled = true
ppImgCI.numDimensions = 2
ppImgCI.width = w; ppImgCI.height = h; ppImgCI.depth = 1
ppImgCI.mipLevels = 1; ppImgCI.arrayLayers = 1
local ppImg = rs.createImage(ppImgCI)

local ppViewCI = ImageViewCreateInfo.new()
ppViewCI.aspectMask = "color"
ppViewCI.levelCount = 1; ppViewCI.arrayLayerCount = 1
local ppView = rs.createImageView(ppImg, ppViewCI)

local ppVert = rs.createShaderModule("Shaders/postProcess.vert", "vertex")
local ppFrag = rs.createShaderModule("Shaders/postProcess.frag", "fragment")

local ppPlCI = PipelineLayoutCreateInfo.new()
ppPlCI:addSetLayout(ppDS)
local ppPL = rs.createPipelineLayout(ppPlCI)

local ppPipeCI = PipelineCreateInfo.new()
ppPipeCI.vertexShaderModule   = ppVert
ppPipeCI.fragmentShaderModule = ppFrag
ppPipeCI.pipelineLayout       = ppPL
ppPipeCI.sampleCount          = 1
ppPipeCI.sampleShadingEnable  = false
ppPipeCI.depthTestEnable      = false
ppPipeCI.depthWriteEnable     = false
ppPipeCI.depthBoundsTestEnable = false
ppPipeCI.stencilTestEnable    = false
ppPipeCI.depthCompareOp       = "less"
ppPipeCI.depthAttachmentFormat = "undefined"
ppPipeCI.polygonMode          = "fill"
ppPipeCI:addColorAttachment("r8g8b8a8Unorm")
local ppPipe = rs.createPipeline(ppPipeCI)
rs.destroyShaderModule(ppVert)
rs.destroyShaderModule(ppFrag)

-- ---- Per-frame callback --------------------------------------------------
rs.setRenderFrameCallback(function(frameIndex)
    local sw = rs.getSwapchainWidth()
    local sh = rs.getSwapchainHeight()

    -- Helper: build a 64-byte LuaBuffer containing the 16 floats of a drawable's transform
    local function makeTransformPC(d)
        local pc = LuaBuffer.new()
        pc:resize(64)
        for i = 1, 16 do
            pc:setFloat((i - 1) * 4, d:getTransformAt(i))
        end
        return pc
    end

    local globalDS   = rs.getGlobalDescriptorSet(frameIndex)
    local lightsDS   = rs.getLightsDescriptorSet(frameIndex)
    local materialsDS = rs.getMaterialsDescriptorSet()
    local clusterDS  = rs.getClusterDescriptorSet(frameIndex)
    local drawables  = rs.getSceneDrawables()

    -- ---- Cluster compute (inline, no separate CB) ----------------------
    rs.cmdUsePipeline(clusterPipe)
    rs.cmdBindDescriptorSets("compute", clusterPL, 0,
        {globalDS, lightsDS, clusterDS}, {})
    local zBuf = LuaBuffer.new()
    zBuf:resize(16)
    zBuf:setFloat(0,  rs.getNearPlane())
    zBuf:setFloat(4,  rs.getFarPlane())
    zBuf:setFloat(8,  0.0)
    zBuf:setFloat(12, 0.0)
    rs.cmdPushConstants(clusterPL, "compute", 0, zBuf)
    rs.cmdDispatch(32, 32, 4)
    -- Memory barrier: compute writes -> fragment reads
    rs.cmdGlobalMemoryBarrier("computeShader", "fragmentShader", "shaderWrite", "shaderRead")

    -- ---- Depth pre-pass ------------------------------------------------
    rs.cmdImageBarrier(depthImg,
        "topOfPipe", "earlyFragmentTests",
        "none", "depthStencilAttachmentWrite",
        "undefined", "depthAttachmentOptimal", "depth")

    local preRI = RenderingInfo.new()
    local preRect = Rect2D.new()
    preRect.width = sw; preRect.height = sh
    preRect.offsetX = 0; preRect.offsetY = 0
    preRI:setRenderArea(preRect)
    preRI:setDepthAttachment(depthView, "depthAttachmentOptimal", "clear", "store", 1.0)
    rs.beginRendering(preRI)

    rs.cmdUsePipeline(prePipe)
    local preVP = ViewportInfo.new()
    preVP.width = sw; preVP.height = sh
    preVP.offsetX = 0; preVP.offsetY = 0
    preVP.minDepth = 0.0; preVP.maxDepth = 1.0
    rs.setActiveViewport(0, preVP)
    local preSC = Rect2D.new()
    preSC.width = sw; preSC.height = sh
    preSC.offsetX = 0; preSC.offsetY = 0
    rs.setActiveScissor(0, preSC)

    rs.cmdBindDescriptorSets("graphics", prePL, 0, {globalDS}, {})
    for i = 1, #drawables do
        local d = drawables[i]
        rs.cmdPushConstants(prePL, "vertex", 0, makeTransformPC(d))
        rs.cmdBindVertexBuffer(d.vertexBuffer, 0, 0)
        rs.cmdBindIndexBuffer(d.indexBuffer, 0, d.indexType)
        rs.cmdDrawIndexed(d.indexCount, d.instanceCount, 0, 0, 0)
    end
    rs.endRendering()

    -- Barrier: depth pre-pass writes -> forward depth reads
    rs.cmdImageBarrier(depthImg,
        "lateFragmentTests", "earlyFragmentTests",
        "depthStencilAttachmentWrite", "depthStencilAttachmentRead",
        "depthAttachmentOptimal", "depthAttachmentOptimal", "depth")

    -- ---- Forward pass --------------------------------------------------
    rs.cmdImageBarrier(colorImg,
        "topOfPipe", "colorAttachmentOutput",
        "none", "colorAttachmentWrite",
        "undefined", "colorAttachmentOptimal", "color")
    rs.cmdImageBarrier(normalImg,
        "topOfPipe", "colorAttachmentOutput",
        "none", "colorAttachmentWrite",
        "undefined", "colorAttachmentOptimal", "color")

    local fwdRI = RenderingInfo.new()
    local fwdRect = Rect2D.new()
    fwdRect.width = sw; fwdRect.height = sh
    fwdRect.offsetX = 0; fwdRect.offsetY = 0
    fwdRI:setRenderArea(fwdRect)
    fwdRI:addColorAttachment(colorView,  "colorAttachmentOptimal", "clear", "store", 0, 0, 0, 1)
    fwdRI:addColorAttachment(normalView, "colorAttachmentOptimal", "clear", "store", 0, 0, 0, 1)
    fwdRI:setDepthAttachment(depthView, "depthAttachmentOptimal", "load", "store", 1.0)
    rs.beginRendering(fwdRI)

    rs.cmdUsePipeline(fwdPipe)
    local fwdVP = ViewportInfo.new()
    fwdVP.width = sw; fwdVP.height = sh
    fwdVP.offsetX = 0; fwdVP.offsetY = 0
    fwdVP.minDepth = 0.0; fwdVP.maxDepth = 1.0
    rs.setActiveViewport(0, fwdVP)
    local fwdSC = Rect2D.new()
    fwdSC.width = sw; fwdSC.height = sh
    fwdSC.offsetX = 0; fwdSC.offsetY = 0
    rs.setActiveScissor(0, fwdSC)

    rs.cmdBindDescriptorSets("graphics", fwdPL, 0, {globalDS, lightsDS}, {})
    for i = 1, #drawables do
        local d = drawables[i]
        rs.cmdBindDescriptorSets("graphics", fwdPL, 2, {materialsDS}, {d.materialDynamicOffset})
        rs.cmdPushConstants(fwdPL, "vertex", 0, makeTransformPC(d))
        rs.cmdBindVertexBuffer(d.vertexBuffer, 0, 0)
        rs.cmdBindIndexBuffer(d.indexBuffer, 0, d.indexType)
        rs.cmdDrawIndexed(d.indexCount, d.instanceCount, 0, 0, 0)
    end
    rs.endRendering()

    -- ---- Post-process --------------------------------------------------
    rs.cmdImageBarrier(colorImg,
        "colorAttachmentOutput", "fragmentShader",
        "colorAttachmentWrite",  "shaderRead",
        "colorAttachmentOptimal", "shaderReadOnlyOptimal", "color")
    rs.cmdImageBarrier(depthImg,
        "lateFragmentTests", "fragmentShader",
        "depthStencilAttachmentWrite", "shaderRead",
        "depthAttachmentOptimal", "shaderReadOnlyOptimal", "depth")
    rs.cmdImageBarrier(normalImg,
        "colorAttachmentOutput", "fragmentShader",
        "colorAttachmentWrite",  "shaderRead",
        "colorAttachmentOptimal", "shaderReadOnlyOptimal", "color")

    rs.cmdImageBarrier(ppImg,
        "topOfPipe", "colorAttachmentOutput",
        "none", "colorAttachmentWrite",
        "undefined", "colorAttachmentOptimal", "color")

    local ppRI = RenderingInfo.new()
    local ppRect = Rect2D.new()
    ppRect.width = sw; ppRect.height = sh
    ppRect.offsetX = 0; ppRect.offsetY = 0
    ppRI:setRenderArea(ppRect)
    ppRI:addColorAttachment(ppView, "colorAttachmentOptimal", "dontCare", "store", 0, 0, 0, 1)
    rs.beginRendering(ppRI)

    rs.cmdUsePipeline(ppPipe)
    local ppVP = ViewportInfo.new()
    ppVP.width = sw; ppVP.height = sh
    ppVP.offsetX = 0; ppVP.offsetY = 0
    ppVP.minDepth = 0.0; ppVP.maxDepth = 1.0
    rs.setActiveViewport(0, ppVP)
    local ppSC = Rect2D.new()
    ppSC.width = sw; ppSC.height = sh
    ppSC.offsetX = 0; ppSC.offsetY = 0
    rs.setActiveScissor(0, ppSC)

    rs.cmdBindDescriptorSets("graphics", ppPL, 0, {ppDS}, {})
    rs.cmdDraw(3, 1, 0, 0)
    rs.endRendering()

    -- Blit ppImg -> swapchain
    rs.cmdImageBarrier(ppImg,
        "colorAttachmentOutput", "transfer",
        "colorAttachmentWrite", "transferRead",
        "colorAttachmentOptimal", "transferSrcOptimal", "color")

    local swapInfo = rs.acquireNextSwapchainImage(0)

    rs.cmdImageBarrier(swapInfo.swapchainImage,
        "topOfPipe", "transfer",
        "none", "transferWrite",
        "undefined", "transferDstOptimal", "color")

    local blit = BlitImageInfo.new()
    blit.aspectMask = "color"
    blit.filter     = "nearest"
    blit.srcLayout  = "transferSrcOptimal"
    blit.dstLayout  = "transferDstOptimal"
    blit.srcX1 = sw; blit.srcY1 = sh
    blit.dstX1 = rs.getSwapchainWidth(); blit.dstY1 = rs.getSwapchainHeight()
    rs.cmdBlitImage(ppImg, swapInfo.swapchainImage, blit)

    rs.cmdImageBarrier(swapInfo.swapchainImage,
        "transfer", "colorAttachmentOutput",
        "transferWrite", "colorAttachmentWrite",
        "transferDstOptimal", "colorAttachmentOptimal", "color")
    -- print("apple")
    -- rs.log("Rendering frame " .. frameIndex)
end)
