#include "VulkanStartingOut.hpp"
#include "CubeVoxel.h"

int main() {
	Application& app = Application::app();
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << "TOP LEVEL ERROR: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}




/*
	.Make the core class, which contains VkDevice, VmaAllocator, etc.
	.Make the other classes for Buffer, Semaphore, Fence, CommandPool that take core in the constructor
	.Store pointer to core everywhere

*/