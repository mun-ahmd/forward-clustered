--[[
  Standalone cloud demo: 3D noise in a storage buffer, compute fills an HDR-ish RGBA buffer,
  fullscreen pass tonemaps to LDR and blits to the swapchain.

  Point master.lua / scene metadata at scripts/rendering/cloud_rendering.lua. Self-contained (no rendering.lua hook).

  3D noise uses a storage buffer (no VkImage3D) because the Lua descriptor pool has no storageImage slots.
]]

local rs = rendering

-- First acquire of each swapchain image index is UNDEFINED; after present it is PRESENT_SRC_KHR.
local swapSeenForBlit = {}

local kNoiseN = 32
local kNoiseVoxels = kNoiseN * kNoiseN * kNoiseN
local noiseByteSize = kNoiseVoxels * 4

local function fract(x)
	return x - math.floor(x)
end

local function nhash(ix, iy, iz)
	local s = math.sin(ix * 12.9898 + iy * 78.233 + iz * 45.164 + 91.345) * 43758.5453
	return fract(s)
end

local w = rs.getSwapchainWidth()
local h = rs.getSwapchainHeight()

-- After frame 0, ppImg ends in transferSrcOptimal; beginRendering must transition from that, not undefined.
local ppLayoutFrame = 0

-- ---- 3D noise buffer (GPU): filled once from a host-mapped staging buffer ------------
local noiseStagingCI = BufferCreateInfo.new()
noiseStagingCI.size = noiseByteSize
noiseStagingCI.createDedicatedMemory = false
noiseStagingCI.createMapped = true
local noiseStaging = rs.createBuffer(noiseStagingCI)

local z = 0
for iz = 0, kNoiseN - 1 do
	for iy = 0, kNoiseN - 1 do
		for ix = 0, kNoiseN - 1 do
			local v = nhash(ix, iy, iz)
			rs.writeFloatToBuffer(noiseStaging, z, v)
			z = z + 4
		end
	end
end

local noiseBufCI = BufferCreateInfo.new()
noiseBufCI.size = noiseByteSize
noiseBufCI.createDedicatedMemory = false
noiseBufCI.createMapped = false
local noiseBuf = rs.createBuffer(noiseBufCI)

local noiseCopy = BufferCopyInfo.new()
noiseCopy.srcBuffer = noiseStaging
noiseCopy.dstBuffer = noiseBuf
noiseCopy.srcOffset = 0
noiseCopy.dstOffset = 0
noiseCopy.size = noiseByteSize
rs.copyBuffer(noiseCopy)
rs.destroyBuffer(noiseStaging)

-- ---- Cloud RGBA output (per swapchain pixel, vec4) ------------------------------------
local outByteSize = w * h * 16
local outBufCI = BufferCreateInfo.new()
outBufCI.size = outByteSize
outBufCI.createDedicatedMemory = false
outBufCI.createMapped = false
local outBuf = rs.createBuffer(outBufCI)

-- ---- Descriptor pool: two sets, three storage-buffer descriptors total ----------------
-- RenderingServer always registers four pool sizes; each must be >0 for valid VkDescriptorPool.
local poolCI = DescriptorPoolCreateInfo.new()
poolCI.maxSets = 2
poolCI.uniformBufferCount = 1
poolCI.uniformBufferDynamicCount = 1
poolCI.storageBufferCount = 3
poolCI.combinedImageSamplerCount = 1
local cloudPool = rs.createDescriptorPool(poolCI)

local compDsCI = DescriptorSetCreateInfo.new()
compDsCI.descriptorPool = cloudPool
compDsCI:addBinding(0, 1, "storageBuffer", "compute", false)
compDsCI:addBinding(1, 1, "storageBuffer", "compute", false)
local compDS = rs.createDescriptorSet(compDsCI)
rs.writeBufferToDescriptorSet(compDS, 0, 0, noiseBuf, 0, noiseByteSize, false, false)
rs.writeBufferToDescriptorSet(compDS, 1, 0, outBuf, 0, outByteSize, false, false)

local presentDsCI = DescriptorSetCreateInfo.new()
presentDsCI.descriptorPool = cloudPool
presentDsCI:addBinding(0, 1, "storageBuffer", "fragment", false)
local presentDS = rs.createDescriptorSet(presentDsCI)
rs.writeBufferToDescriptorSet(presentDS, 0, 0, outBuf, 0, outByteSize, false, false)

-- ---- Compute: clouds ------------------------------------------------------------------
local compShader = rs.createShaderModule("Shaders/clouds_compute.comp", "compute")

local compPlCI = PipelineLayoutCreateInfo.new()
compPlCI:addSetLayout(compDS)
compPlCI:addPushConstant(0, 16, "compute")
local compPL = rs.createPipelineLayout(compPlCI)

local compPipeCI = ComputePipelineCreateInfo.new()
compPipeCI.computeShaderModule = compShader
compPipeCI.pipelineLayout = compPL
local compPipe = rs.createComputePipeline(compPipeCI)
rs.destroyShaderModule(compShader)

-- ---- Present: buffer -> LDR color attachment ----------------------------------------
local ppImgCI = ImageCreateInfo.new()
ppImgCI.format = "r8g8b8a8Unorm"
ppImgCI.isColorAttachment = true
ppImgCI.isSampled = true
ppImgCI.isStorage = false
ppImgCI.numDimensions = 2
ppImgCI.width = w
ppImgCI.height = h
ppImgCI.depth = 1
ppImgCI.mipLevels = 1
ppImgCI.arrayLayers = 1
local ppImg = rs.createImage(ppImgCI)

local ppViewCI = ImageViewCreateInfo.new()
ppViewCI.aspectMask = "color"
ppViewCI.levelCount = 1
ppViewCI.arrayLayerCount = 1
ppViewCI.viewType = "2d"
local ppView = rs.createImageView(ppImg, ppViewCI)

local pv = rs.createShaderModule("Shaders/cloud_present.vert", "vertex")
local pf = rs.createShaderModule("Shaders/cloud_present.frag", "fragment")

local presPlCI = PipelineLayoutCreateInfo.new()
presPlCI:addSetLayout(presentDS)
presPlCI:addPushConstant(0, 8, "fragment")
local presPL = rs.createPipelineLayout(presPlCI)

local presPipeCI = PipelineCreateInfo.new()
presPipeCI.vertexShaderModule = pv
presPipeCI.fragmentShaderModule = pf
presPipeCI.pipelineLayout = presPL
presPipeCI.sampleCount = 1
presPipeCI.sampleShadingEnable = false
presPipeCI.depthTestEnable = false
presPipeCI.depthWriteEnable = false
presPipeCI.depthBoundsTestEnable = false
presPipeCI.stencilTestEnable = false
presPipeCI.depthCompareOp = "less"
presPipeCI.depthAttachmentFormat = "undefined"
presPipeCI.polygonMode = "fill"
presPipeCI:addColorAttachment("r8g8b8a8Unorm")
local presPipe = rs.createPipeline(presPipeCI)
rs.destroyShaderModule(pv)
rs.destroyShaderModule(pf)

-- ---- Per-frame ------------------------------------------------------------------------------
local cloudTime0 = os.clock()
rs.setRenderFrameCallback(function(frameIndex)
	local sw = math.min(rs.getSwapchainWidth(), w)
	local sh = math.min(rs.getSwapchainHeight(), h)

	local gx = math.max(1, math.floor((sw + 7) / 8))
	local gy = math.max(1, math.floor((sh + 7) / 8))

	local compPush = LuaBuffer.new()
	compPush:resize(16)
	compPush:setFloat(0, os.clock() - cloudTime0)
	compPush:setFloat(4, 0.0)
	compPush:setFloat(8, sw + 0.0)
	compPush:setFloat(12, sh + 0.0)

	rs.cmdUsePipeline(compPipe)
	rs.cmdBindDescriptorSets("compute", compPL, 0, { compDS }, {})
	rs.cmdPushConstants(compPL, "compute", 0, compPush)
	rs.cmdDispatch(gx, gy, 1)

	rs.cmdGlobalMemoryBarrier("computeShader", "fragmentShader", "shaderWrite", "shaderRead")

	if ppLayoutFrame > 0 then
		rs.cmdImageBarrier(ppImg,
			"transfer", "colorAttachmentOutput",
			"transferRead", "colorAttachmentWrite",
			"transferSrcOptimal", "colorAttachmentOptimal", "color")
	else
		rs.cmdImageBarrier(ppImg,
			"topOfPipe", "colorAttachmentOutput",
			"none", "colorAttachmentWrite",
			"undefined", "colorAttachmentOptimal", "color")
	end

	local ri = RenderingInfo.new()
	local rect = Rect2D.new()
	rect.width = sw
	rect.height = sh
	rect.offsetX = 0
	rect.offsetY = 0
	ri:setRenderArea(rect)
	ri:addColorAttachment(ppView, "colorAttachmentOptimal", "dontCare", "store", 0, 0, 0, 1)
	rs.beginRendering(ri)

	rs.cmdUsePipeline(presPipe)
	local vp = ViewportInfo.new()
	vp.width = sw
	vp.height = sh
	vp.offsetX = 0
	vp.offsetY = 0
	vp.minDepth = 0.0
	vp.maxDepth = 1.0
	rs.setActiveViewport(0, vp)
	local sc = Rect2D.new()
	sc.width = sw
	sc.height = sh
	sc.offsetX = 0
	sc.offsetY = 0
	rs.setActiveScissor(0, sc)

	local presPush = LuaBuffer.new()
	presPush:resize(8)
	presPush:setFloat(0, sw + 0.0)
	presPush:setFloat(4, sh + 0.0)

	rs.cmdBindDescriptorSets("graphics", presPL, 0, { presentDS }, {})
	rs.cmdPushConstants(presPL, "fragment", 0, presPush)
	rs.cmdDraw(3, 1, 0, 0)
	rs.endRendering()

	rs.cmdImageBarrier(ppImg,
		"colorAttachmentOutput", "transfer",
		"colorAttachmentWrite", "transferRead",
		"colorAttachmentOptimal", "transferSrcOptimal", "color")

	local swapInfo = rs.acquireNextSwapchainImage(0)
	local si = swapInfo.imageIndex
	if swapSeenForBlit[si] then
		rs.cmdImageBarrier(swapInfo.swapchainImage,
			"bottomOfPipe", "transfer",
			"none", "transferWrite",
			"presentSrcKhr", "transferDstOptimal", "color")
	else
		rs.cmdImageBarrier(swapInfo.swapchainImage,
			"topOfPipe", "transfer",
			"none", "transferWrite",
			"undefined", "transferDstOptimal", "color")
	end
	swapSeenForBlit[si] = true

	local blit = BlitImageInfo.new()
	blit.aspectMask = "color"
	blit.filter = "nearest"
	blit.srcLayout = "transferSrcOptimal"
	blit.dstLayout = "transferDstOptimal"
	blit.srcX1 = sw
	blit.srcY1 = sh
	blit.dstX1 = rs.getSwapchainWidth()
	blit.dstY1 = rs.getSwapchainHeight()
	rs.cmdBlitImage(ppImg, swapInfo.swapchainImage, blit)

	rs.cmdImageBarrier(swapInfo.swapchainImage,
		"transfer", "colorAttachmentOutput",
		"transferWrite", "colorAttachmentWrite",
		"transferDstOptimal", "colorAttachmentOptimal", "color")

	ppLayoutFrame = ppLayoutFrame + 1
end)
