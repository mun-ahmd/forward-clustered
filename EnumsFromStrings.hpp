#pragma once
#include <string>
#include <vulkan/vulkan.hpp>

namespace Rendering {
	VkImageLayout layoutFromString(std::string layout);
	VkFormat formatFromString(std::string format);
	VkPolygonMode polygonModeFromString(std::string mode);
	VkCompareOp compareOpFromString(std::string op);
	VkShaderStageFlags shaderStageFlagsFromString(std::string shaderStages);
	VkImageAspectFlags imageAspectFromString(std::string aspect) {
		//only 3 image aspects supported right now
		//can only be used seperately not properly as a mask
		// so big limitation for some uses of depth_stencil images already
		//todo maybe I can add masks by parsing strings like: color_depth_stencil as colorFlag | depthFlag | stencilFlag

		if (aspect == "color") {
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (aspect == "depth") {
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		else if (aspect == "stencil") {
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			std::cerr << "image aspect: " << aspect << " not supported" << std::endl;
			assert(false);
			return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
		}
	}
}