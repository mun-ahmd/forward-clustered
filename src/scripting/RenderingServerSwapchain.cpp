// Swapchain-dependent RenderingServer methods.
// Kept in a separate translation unit because core/swapchain.hpp pulls in
// core/image.hpp which declares a global ::Image class that would clash with
// Rendering::Image under "using namespace Rendering;" used in RenderingServer.cpp.
#include "core/swapchain.hpp"
#include "scripting/RenderingServer.hpp"
#include "scripting/EnumsFromStrings.hpp"

void RenderingServer::connectSwapchain(SwapChain* swapchain) {
	// Evict previous swapchain resource IDs without destroying the underlying
	// Vulkan objects (SwapChain owns those).
	for (Rendering::ResourceID id : swapchainImageViewIDs) {
		this->resources.remove<Rendering::ImageView>(id);
	}
	for (Rendering::ResourceID id : swapchainImageIDs) {
		this->resources.remove<Rendering::Image>(id);
	}
	swapchainImageIDs.clear();
	swapchainImageViewIDs.clear();

	connectedSwapchain = swapchain;

	// Register as OTHER-tagged so they survive LUA script reloads.
	setActiveTagForAll(static_cast<uint8_t>(ResourceUser::OTHER));

	for (size_t i = 0; i < swapchain->swapChainImages.size(); i++) {
		Rendering::Image img{};
		img.image         = swapchain->swapChainImages[i];
		img.format        = swapchain->swapChainImageFormat;
		img.imageType     = VK_IMAGE_TYPE_2D;
		img.extent        = { swapchain->swapChainExtent.width, swapchain->swapChainExtent.height, 1 };
		img.mipLevels     = 1;
		img.arrayLayers   = 1;
		img.layout        = VK_IMAGE_LAYOUT_UNDEFINED;
		img.allocation    = VK_NULL_HANDLE;
		img.isSwapchainImage = true;
		swapchainImageIDs.push_back(this->resources.add(img));

		Rendering::ImageView view{};
		view.view = swapchain->swapChainImageViews[i];
		swapchainImageViewIDs.push_back(this->resources.add(view));
	}

	setActiveTagForAll(static_cast<uint8_t>(activeResourceUser));
}

uint32_t RenderingServer::getSwapchainWidth() {
	assert(connectedSwapchain);
	return connectedSwapchain->swapChainExtent.width;
}

uint32_t RenderingServer::getSwapchainHeight() {
	assert(connectedSwapchain);
	return connectedSwapchain->swapChainExtent.height;
}

std::string RenderingServer::getSwapchainFormat() {
	assert(connectedSwapchain);
	return Rendering::formatToString(connectedSwapchain->swapChainImageFormat);
}

Rendering::FrameStartInfo RenderingServer::acquireNextSwapchainImage(Rendering::ResourceID /*semaphoreID*/) {
	assert(connectedSwapchain);
	assert(currentSwapchainImageIndex < swapchainImageIDs.size());

	Rendering::FrameStartInfo info{};
	info.swapchainImage     = swapchainImageIDs[currentSwapchainImageIndex];
	info.swapchainImageView = swapchainImageViewIDs[currentSwapchainImageIndex];
	info.imageIndex         = currentSwapchainImageIndex;
	info.swapchainWidth     = connectedSwapchain->swapChainExtent.width;
	info.swapchainHeight    = connectedSwapchain->swapChainExtent.height;
	return info;
}

void RenderingServer::presentSwapchainImage(uint32_t /*imageIndex*/, Rendering::ResourceID /*semaphoreID*/) {
	// Phase 2: no-op. Frame::performFrame handles present.
}
