//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//
//#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
//
//#include <shaderc/shaderc.hpp>
//
//#include <iostream>
//#include <vector>
//#include <set>
//#include <optional>
//#include <algorithm>
//#include <fstream>
//#include <filesystem>
//
//const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
//
//const std::vector<const char*> deviceExtensions = {
//	VK_KHR_SWAPCHAIN_EXTENSION_NAME
//};
//
//#ifdef NDEBUG
//const bool enableValidationLayers = false;
//#else
//const bool enableValidationLayers = true;
//#endif
//
//class QueueFamilyIndices
//{
//public:
//	std::optional<uint32_t> graphicsFamily;
//	std::optional<uint32_t> presentFamily;
//
//	bool isComplete()
//	{
//		return (graphicsFamily.has_value() && presentFamily.has_value());
//	}
//};
//
//struct SwapChainSupportDetails {
//	VkSurfaceCapabilitiesKHR capabilities;
//	std::vector<VkSurfaceFormatKHR> formats;
//	std::vector<VkPresentModeKHR> presentModes;
//};
//
//std::string readFile(std::string filePath) {
//	std::ifstream file;
//	std::string text;
//	file.open(filePath);
//	if (!file.is_open())
//		throw std::runtime_error("Error opening file: " + filePath);
//	file.seekg(0, std::ios::end);
//	text.reserve(file.tellg());
//	file.seekg(0, std::ios::beg);
//	text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//	file.close();
//	return text;
//}
//
//class Pipeline {
//private:
//	std::string dirPath;
//	VkPipelineLayout pipelineLayout;
//	std::vector<uint32_t> compileGlslToSpv(std::string sourceFile, std::string compiledDest, shaderc_shader_kind shaderKind, const shaderc::Compiler& compiler) {
//		std::string sourceCode = readFile(sourceFile);
//		shaderc::CompileOptions options;
//		auto compilationResults = compiler.CompileGlslToSpv(sourceCode, shaderKind, sourceFile.c_str());
//		if (compilationResults.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
//			std::cerr << "Error Compiling " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
//			std::cerr << compilationResults.GetErrorMessage();
//			throw std::runtime_error("Error Compiling " + sourceFile);
//		}
//		std::cout << "Successfully Compiled " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
//		return std::vector<uint32_t>(compilationResults.begin(), compilationResults.end());
//	}
//
//	VkShaderModule createShaderModule(const std::vector<uint32_t>& code, const VkDevice& device) {
//		VkShaderModuleCreateInfo createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//		createInfo.codeSize = code.size();
//		createInfo.pCode = code.data();
//		VkShaderModule shaderModule;
//		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create shader module!");
//		}
//		return shaderModule;
//	}
//
//public:
//
//	Pipeline(std::string name, std::string vertexPath, std::string fragPath, const shaderc::Compiler& compiler, const VkDevice& device) : dirPath("./CompiledShaders/" + name) {
//		if (!std::filesystem::create_directory(dirPath))
//			throw std::runtime_error("Could not create directory for compiled shader");
//		auto vertCode = compileGlslToSpv(vertexPath, dirPath + "/" + "vert.spv", shaderc_shader_kind::shaderc_vertex_shader, compiler);
//		auto fragCode = compileGlslToSpv(fragPath, dirPath + "/" + "frag.spv", shaderc_shader_kind::shaderc_fragment_shader, compiler);
//
//		auto vertModule = createShaderModule(vertCode, device);
//		auto fragModule = createShaderModule(fragCode, device);
//
//		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//		vertShaderStageInfo.module = vertModule;
//		vertShaderStageInfo.pName = "main";
//
//		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//		fragShaderStageInfo.module = fragModule;
//		fragShaderStageInfo.pName = "main";
//
//		std::vector<VkDynamicState> dynamicStates = {
//			VK_DYNAMIC_STATE_VIEWPORT,
//			VK_DYNAMIC_STATE_SCISSOR
//		};
//
//		VkPipelineDynamicStateCreateInfo dynamicState{};
//		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
//		dynamicState.pDynamicStates = dynamicStates.data();
//
//		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//		vertexInputInfo.vertexBindingDescriptionCount = 0;
//		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
//		vertexInputInfo.vertexAttributeDescriptionCount = 0;
//		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
//
//		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//		inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//		VkPipelineViewportStateCreateInfo viewportState{};
//		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//		viewportState.viewportCount = 1;
//		viewportState.scissorCount = 1;
//
//		VkPipelineRasterizationStateCreateInfo rasterizer{};
//		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//		rasterizer.depthClampEnable = VK_FALSE;
//		rasterizer.rasterizerDiscardEnable = VK_FALSE;
//		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//		rasterizer.lineWidth = 1.0f;
//
//		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//
//		rasterizer.depthBiasEnable = VK_FALSE;
//		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
//		rasterizer.depthBiasClamp = 0.0f; // Optional
//		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
//
//		VkPipelineMultisampleStateCreateInfo multisampling{};
//		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//		multisampling.sampleShadingEnable = VK_FALSE;
//		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//		multisampling.minSampleShading = 1.0f; // Optional
//		multisampling.pSampleMask = nullptr; // Optional
//		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//		multisampling.alphaToOneEnable = VK_FALSE; // Optional
//
//		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//		colorBlendAttachment.blendEnable = VK_FALSE;
//		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
//
//		VkPipelineColorBlendStateCreateInfo colorBlending{};
//		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//		colorBlending.logicOpEnable = VK_FALSE;
//		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
//		colorBlending.attachmentCount = 1;
//		colorBlending.pAttachments = &colorBlendAttachment;
//		colorBlending.blendConstants[0] = 0.0f; // Optional
//		colorBlending.blendConstants[1] = 0.0f; // Optional
//		colorBlending.blendConstants[2] = 0.0f; // Optional
//		colorBlending.blendConstants[3] = 0.0f; // Optional
//
//		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//		pipelineLayoutInfo.setLayoutCount = 0; // Optional
//		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
//		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
//
//		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create pipeline layout!");
//		}
//	}
//
//};
//
//class VulkanApplication
//{
//private:
//	GLFWwindow* window;
//	shaderc::Compiler compiler = shaderc::Compiler();
//
//
//	VkInstance instance;
//	VkDebugUtilsMessengerEXT debugmessenger;
//	VkSurfaceKHR surface;
//
//	VkPhysicalDevice physicalDevice;
//	VkDevice device;
//
//	QueueFamilyIndices families;
//
//	VkQueue graphicsqueue;
//	VkQueue presentqueue;
//
//	VkSwapchainKHR swapChain;
//	std::vector<VkImage> swapChainImages;
//	VkFormat swapChainImageFormat;
//	VkExtent2D swapChainExtent;
//
//	std::vector<VkImageView> swapChainImageViews;
//
//	bool windowinitialized = false;
//
//	template <class func_to_load, class return_type_of_func, class... func_arg_types>
//	return_type_of_func loadAndExecuteExtFunc(VkInstance instance, const char* func_name, func_arg_types... func_args)
//	{
//		auto func = (func_to_load)vkGetInstanceProcAddr(instance, func_name);
//		if (func != nullptr)
//		{
//			return func(func_args...);
//		}
//		else
//		{
//			//error handle
//			exit(1);
//		}
//	}
//
//	void createInstance()
//	{
//		VkApplicationInfo appInfo
//		{
//			VK_STRUCTURE_TYPE_APPLICATION_INFO,
//			nullptr,
//			"VulkanStartingOut",
//			VK_MAKE_VERSION(1,0,0),
//			"NoEngine",
//			VK_MAKE_VERSION(1,0,0),
//			VK_API_VERSION_1_0
//		};
//
//		uint32_t glfwExtentionsCount = 0;
//		const char** glfwExtentions = glfwGetRequiredInstanceExtensions(&glfwExtentionsCount);
//		std::vector<const char*> extensions(glfwExtentions, glfwExtentions + glfwExtentionsCount);
//		const char* const* layers = nullptr;
//		uint32_t numlayers = 0;
//		if (enableValidationLayers)
//		{
//			numlayers = validation_layers.size();
//			layers = validation_layers.data();
//			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//		}
//		VkInstanceCreateInfo instanceCreateInfo
//		{
//			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//			nullptr,
//			0,
//			&appInfo,
//			numlayers,            //num validation layers rn
//			layers,            //validation layers
//			extensions.size(),
//			extensions.data()
//		};
//
//		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
//		if (result != VK_SUCCESS || result == VK_ERROR_LAYER_NOT_PRESENT)
//		{
//			throw std::runtime_error("failed to create instance");
//		}
//	}
//
//	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
//		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//		VkDebugUtilsMessageTypeFlagsEXT messageType,
//		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//		void* pUserData) {
//
//		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//		return VK_FALSE;
//	}
//
//	void setupDebugger()
//	{
//		if (!enableValidationLayers) return;
//		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
//		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
//		createInfo.pfnUserCallback = debugCallback;
//		createInfo.pUserData = nullptr; // Optional
//		VkResult result = loadAndExecuteExtFunc<PFN_vkCreateDebugUtilsMessengerEXT, VkResult>(instance, "vkCreateDebugUtilsMessengerEXT", instance, &createInfo, nullptr, &debugmessenger);
//		if (result != VK_SUCCESS)
//		{
//			throw std::runtime_error("error creating vkDebugUtilsMessenger");
//		}
//	}
//
//	void createSurface()
//	{
//		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create window surface!");
//		}
//	}
//
//	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice pDevice)
//	{
//		QueueFamilyIndices indices;
//		uint32_t numFamilies = 0;
//		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numFamilies, nullptr);
//		std::vector<VkQueueFamilyProperties> families(numFamilies);
//		vkGetPhysicalDeviceQueueFamilyProperties(pDevice, &numFamilies, families.data());
//
//		int i = 0;
//		for (const auto& queueFamily : families) {
//			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
//				indices.graphicsFamily = i;
//			}
//			VkBool32 presentSupport = false;
//			vkGetPhysicalDeviceSurfaceSupportKHR(pDevice, i, surface, &presentSupport);
//			if (presentSupport)
//			{
//				indices.presentFamily = i;
//			}
//			i++;
//		}
//
//		return indices;
//	}
//
//	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
//		uint32_t extensionCount;
//		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
//
//		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
//
//		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
//
//		for (const auto& extension : availableExtensions) {
//			requiredExtensions.erase(extension.extensionName);
//		}
//
//		return requiredExtensions.empty();
//	}
//
//	bool isPhysDeviceSuitable(VkPhysicalDevice device) {
//		QueueFamilyIndices indices = findQueueFamilies(device);
//
//		bool extensionsSupported = checkDeviceExtensionSupport(device);
//
//		bool swapChainAdequate = false;
//		if (extensionsSupported) {
//			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
//			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
//		}
//
//		return indices.isComplete() && extensionsSupported && swapChainAdequate;
//	}
//
//	void pickPhysicalDevice()
//	{
//		uint32_t numPhysDevices = 0;
//		vkEnumeratePhysicalDevices(instance, &numPhysDevices, nullptr);
//		if (numPhysDevices == 0)
//			throw std::runtime_error("NO PHYSICAL DEVICE WITH VULKAN SUPPORT FOUND");
//		std::vector<VkPhysicalDevice> pDevices(numPhysDevices);
//		vkEnumeratePhysicalDevices(instance, &numPhysDevices, pDevices.data());
//		for (const auto& pDevice : pDevices)
//		{
//			if (isPhysDeviceSuitable(pDevice))
//			{
//				this->physicalDevice = pDevice;
//				break;
//			}
//		}
//	}
//
//	void createLogicalDevice()
//	{
//
//		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//		std::set<uint32_t> uniqueQueueFamilies = { families.graphicsFamily.value(), families.presentFamily.value() };
//
//		float queuePriority = 1.0f;
//		for (uint32_t queueFamily : uniqueQueueFamilies) {
//			VkDeviceQueueCreateInfo queueCreateInfo{};
//			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//			queueCreateInfo.queueFamilyIndex = queueFamily;
//			queueCreateInfo.queueCount = 1;
//			queueCreateInfo.pQueuePriorities = &queuePriority;
//			queueCreateInfos.push_back(queueCreateInfo);
//		}
//
//		VkPhysicalDeviceFeatures deviceFeatures{};
//
//		VkDeviceCreateInfo createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//
//		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//		createInfo.pQueueCreateInfos = queueCreateInfos.data();
//
//		createInfo.pEnabledFeatures = &deviceFeatures;
//
//		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
//		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
//
//		if (enableValidationLayers) {
//			createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
//			createInfo.ppEnabledLayerNames = validation_layers.data();
//		}
//		else {
//			createInfo.enabledLayerCount = 0;
//		}
//
//		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create logical device!");
//		}
//
//		vkGetDeviceQueue(device, families.graphicsFamily.value(), 0, &graphicsqueue);
//		vkGetDeviceQueue(device, families.presentFamily.value(), 0, &presentqueue);
//	}
//
//	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
//		SwapChainSupportDetails details;
//
//		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
//
//		uint32_t formatCount;
//		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
//
//		if (formatCount != 0) {
//			details.formats.resize(formatCount);
//			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
//		}
//
//		uint32_t presentModeCount;
//		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
//
//		if (presentModeCount != 0) {
//			details.presentModes.resize(presentModeCount);
//			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
//		}
//
//		return details;
//	}
//
//	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
//		for (const auto& availableFormat : availableFormats) {
//			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
//				return availableFormat;
//			}
//		}
//
//		return availableFormats[0];
//	}
//
//	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
//		for (const auto& availablePresentMode : availablePresentModes) {
//			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
//				return availablePresentMode;
//			}
//		}
//
//		return VK_PRESENT_MODE_FIFO_KHR;
//	}
//
//	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
//		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//			return capabilities.currentExtent;
//		}
//		else {
//			int width, height;
//			glfwGetFramebufferSize(window, &width, &height);
//
//			VkExtent2D actualExtent = {
//				static_cast<uint32_t>(width),
//				static_cast<uint32_t>(height)
//			};
//
//			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
//			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
//
//			return actualExtent;
//		}
//	}
//
//
//	void createSwapChain() {
//		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
//
//		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
//		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
//		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
//
//		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
//		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
//			imageCount = swapChainSupport.capabilities.maxImageCount;
//		}
//
//		VkSwapchainCreateInfoKHR createInfo{};
//		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//		createInfo.surface = surface;
//
//		createInfo.minImageCount = imageCount;
//		createInfo.imageFormat = surfaceFormat.format;
//		createInfo.imageColorSpace = surfaceFormat.colorSpace;
//		createInfo.imageExtent = extent;
//		createInfo.imageArrayLayers = 1;
//		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
//
//		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
//		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
//
//		if (indices.graphicsFamily != indices.presentFamily) {
//			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//			createInfo.queueFamilyIndexCount = 2;
//			createInfo.pQueueFamilyIndices = queueFamilyIndices;
//		}
//		else {
//			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//		}
//
//		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
//		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
//		createInfo.presentMode = presentMode;
//		createInfo.clipped = VK_TRUE;
//
//		createInfo.oldSwapchain = VK_NULL_HANDLE;
//
//		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &this->swapChain) != VK_SUCCESS) {
//			throw std::runtime_error("failed to create swap chain!");
//		}
//
//		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
//		swapChainImages.resize(imageCount);
//		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
//
//		swapChainImageFormat = surfaceFormat.format;
//		swapChainExtent = extent;
//	}
//
//	void createImageViews() {
//		swapChainImageViews.resize(swapChainImages.size());
//
//		for (size_t i = 0; i < swapChainImages.size(); i++) {
//			VkImageViewCreateInfo createInfo{};
//			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//			createInfo.image = swapChainImages[i];
//			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//			createInfo.format = swapChainImageFormat;
//			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//			createInfo.subresourceRange.baseMipLevel = 0;
//			createInfo.subresourceRange.levelCount = 1;
//			createInfo.subresourceRange.baseArrayLayer = 0;
//			createInfo.subresourceRange.layerCount = 1;
//
//			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
//				throw std::runtime_error("failed to create image views!");
//			}
//		}
//	}
//
//public:
//
//	void initializeWindow()
//	{
//		glfwInit();
//
//		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
//		uint32_t WIDTH_ = 900;
//		uint32_t HEIGHT_ = 900;
//
//		window = glfwCreateWindow(WIDTH_, HEIGHT_, "Vulkan", nullptr, nullptr);
//		windowinitialized = true;
//	}
//
//	void initializeGraphics()
//	{
//		if (!windowinitialized)
//			initializeWindow();
//		createInstance();
//		setupDebugger();
//		createSurface();
//		pickPhysicalDevice();
//		families = findQueueFamilies(physicalDevice);
//		createLogicalDevice();
//
//		createSwapChain();
//		createImageViews();
//
//		Pipeline shader("basicTriangle", "triangleVert.glsl", "triangleFrag.glsl", compiler);
//	}
//
//	void mainLoop() {
//		while (!glfwWindowShouldClose(window)) {
//			glfwPollEvents();
//		}
//	}
//
//	void cleanup()
//	{
//		loadAndExecuteExtFunc<PFN_vkDestroyDebugUtilsMessengerEXT, void>(instance, "vkDestroyDebugUtilsMessengerEXT", instance, debugmessenger, nullptr);
//		for (auto imageView : swapChainImageViews) {
//			vkDestroyImageView(device, imageView, nullptr);
//		}
//		vkDestroySwapchainKHR(device, swapChain, nullptr);
//		vkDestroyDevice(device, nullptr);
//		glfwDestroyWindow(window);
//		vkDestroySurfaceKHR(instance, surface, nullptr);
//		vkDestroyInstance(instance, nullptr);
//	}
//
//};
//
//int main()
//{
//	VulkanApplication app;
//	app.initializeGraphics();
//	app.mainLoop();
//	app.cleanup();
//	return 0;
//}