#pragma once

#include <fstream>

#include <shaderc/shaderc.hpp>
#include "core.hpp"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

//RC = Reference Counted
template<typename T>
using RC = std::shared_ptr<T>;

class VulkanUtils {
	VulkanCore core;
	VkCommandPool commandPool;
	shaderc::Compiler compiler = shaderc::Compiler();

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = core->findQueueFamilies();

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		commandPool = core->createCommandPool(poolInfo);
	}

	VulkanUtils() {
		core = createVulkanCore();
		createCommandPool();
	}
	VulkanUtils(const VulkanUtils&) = delete;
	VulkanUtils(VulkanUtils&&) = delete;
	VulkanUtils& operator=(const VulkanUtils&) = delete;
	VulkanUtils& operator=(VulkanUtils&&) = delete;

public:
	static VulkanUtils& utils() {
		static VulkanUtils utils;
		return utils;
	}

	inline VulkanCore getCore() { return core; }
	inline VkCommandPool getCommandPool() { return commandPool; }

	std::string readFile(std::string filePath) {
		std::ifstream file;
		std::string text;
		file.open(filePath);
		if (!file.is_open())
			throw std::runtime_error("Error opening file: " + filePath);
		file.seekg(0, std::ios::end);
		text.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		text.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		return text;
	}

	std::vector<uint32_t> compileGlslToSpv(std::string sourceFile, shaderc_shader_kind shaderKind) {
		std::string sourceCode = readFile(sourceFile);
		shaderc::CompileOptions options;
		auto compilationResults = compiler.CompileGlslToSpv(sourceCode, shaderKind, sourceFile.c_str());
		if (compilationResults.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success) {
			std::cerr << "Error Compiling " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
			std::cerr << compilationResults.GetErrorMessage();
			throw std::runtime_error("Error Compiling " + sourceFile);
		}
		std::cout << "Successfully Compiled " << sourceFile << " with:\n\t" << compilationResults.GetNumErrors() << " errors\n\t" << compilationResults.GetNumWarnings() << " warnings" << std::endl;
		return std::vector<uint32_t>(compilationResults.begin(), compilationResults.end());
	}

	VkShaderModule createShaderModule(const std::vector<uint32_t>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size() * sizeof(uint32_t);
		createInfo.pCode = code.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(core->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}
};


/*
	future externs:
		glslToSpirvFunc
		spirvToModuleFunc
		VulkanCore 
*/