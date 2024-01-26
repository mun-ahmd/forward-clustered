#pragma once
#include "core.hpp"
#include "swapchain.hpp"

template<class FrameData>
class FrameBase {
private:
	std::optional<std::function<void(FrameData* frame)>> destroyFunc;

	//to avoid reallocating each frame
	std::vector<VkSemaphore> waitSemaphores;
	std::vector<VkShaderStageFlags> waitStageMasks;
	std::vector<VkFence> waitFences;
public:

	VulkanCore core;

	VkCommandPool commandBufferPool;
	VkCommandBuffer commandBuffer;

	VkFence inFlightFence;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;

	//if it does not have a value then frame is not in flight
	std::optional<uint32_t> imageIndex;

	//Frame Data:
	FrameData data;

	FrameBase() = default;

	FrameBase(VulkanCore core, VkCommandPool commandPool, std::function<void(FrameData*)> initializer, std::function<void(FrameData*)> destroyFunc)
		: core(core), commandBufferPool(commandPool), destroyFunc(destroyFunc) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandBufferPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		commandBuffer = core->createCommandBuffer(allocInfo);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		imageAvailableSemaphore = core->createSemaphore(semaphoreInfo);
		renderFinishedSemaphore = core->createSemaphore(semaphoreInfo);
		inFlightFence = core->createFence(fenceInfo);

		initializer(&this->data);
	}

	void performFrame(
		SwapChain& swapChain,
		std::function<void(FrameData&)> dataTransferFunc,
		std::function<void(
			FrameBase<FrameData>&,
			std::vector<VkFence>&,
			std::vector<VkSemaphore>&,
			std::vector<VkPipelineStageFlags>&
			)> performer
	) {
		imageIndex.reset();

		this->waitFences.push_back(this->inFlightFence);
		vkWaitForFences(core->device, this->waitFences.size(), this->waitFences.data(), VK_TRUE, std::numeric_limits<uint64_t>::max());

		//since this frame is currently being presented
		dataTransferFunc(this->data);

		try {
			imageIndex = swapChain.acquireNextImage(core->device, imageAvailableSemaphore);
			vkResetFences(core->device, this->waitFences.size(), this->waitFences.data());
			this->waitFences.clear();
		}
		catch (const OutOfDateSwapChainError& err) {
			this->waitFences.clear();
			throw err;
		}

		vkResetCommandBuffer(commandBuffer, 0);

		this->waitSemaphores.clear();
		this->waitStageMasks.clear();

		performer(*this, this->waitFences, this->waitSemaphores, this->waitStageMasks);

		assert(this->waitSemaphores.size() == this->waitStageMasks.size());
		this->waitSemaphores.push_back(imageAvailableSemaphore);
		this->waitStageMasks.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

		if (!imageIndex.has_value()) {
			//no frame in flight
			throw std::runtime_error("No Frame in Flight!");
		}
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.waitSemaphoreCount = this->waitSemaphores.size();
		submitInfo.pWaitSemaphores = this->waitSemaphores.data();
		submitInfo.pWaitDstStageMask = this->waitStageMasks.data();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(core->graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		swapChain.present(core->presentQueue, signalSemaphores, imageIndex.value());
	}

	void drawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

	}

	void cleanup(const VkDevice& device) {
		if (destroyFunc.has_value())
			destroyFunc.value()(&this->data);
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);
		vkFreeCommandBuffers(device, commandBufferPool, 1, &commandBuffer);
	}
};
