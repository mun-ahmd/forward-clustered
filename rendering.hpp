#pragma once
#include "forwardRenderer.hpp"
#include "postProcessing.hpp"
#include "depthOnlyRenderer.hpp"

constexpr int FRAMES_IN_FLIGHT = 2;

struct globalDescriptor {
	glm::mat4 proj;
	glm::mat4 view;
	glm::mat4 projView;
	glm::vec4 cameraPos_time;
	glm::vec4 fovY_aspectRatio_zNear_zFar;

	void updateValues(
		glm::mat4 proj,
		float fovY,
		float aspectRatio,
		float zNear,
		float zFar,
		glm::mat4 view,
		glm::vec3 cameraPos,
		float time_seconds
	) {
		this->proj = proj;
		this->view = view;
		this->projView = proj * view;
		this->fovY_aspectRatio_zNear_zFar = { fovY, aspectRatio, zNear, zFar };
		this->cameraPos_time = glm::vec4(cameraPos, time_seconds);
	}

	void setView(glm::mat4& view) {
		this->view = view;
		this->projView = proj * view;
	}
};

struct GlobalFrameData {
	RC<Buffer> mappedBuffer;
	VkDescriptorSet descriptor;
	VkDescriptorSetLayout layout;
	void init(VulkanCore core) {
		mappedBuffer = Buffer::create(core, sizeof(globalDescriptor), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, (VmaAllocationCreateFlagBits)(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 0);
		//frame->globalDescBufferMappedPointer = frame->globalDescBuffer->allocation->GetMappedData();

		{
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.stageFlags = VK_SHADER_STAGE_ALL;
			binding.binding = 0;
			descriptor = core->createDescriptorSet({ binding });

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorCount = 1;
			write.descriptorType = binding.descriptorType;
			write.dstSet = descriptor;
			write.dstBinding = binding.binding;
			write.pNext = nullptr;

			VkDescriptorBufferInfo info{};
			info.buffer = mappedBuffer->buffer;
			info.offset = 0;
			info.range = sizeof(globalDescriptor);

			write.pBufferInfo = &info;

			vkUpdateDescriptorSets(core->device, 1, &write, 0, nullptr);
		}
	}
};

struct MeshPushConstants {
	glm::mat4 transform;
};

void forwardRenderScene(
	RC<ForwardRenderer> forwardRenderer,
	std::shared_ptr<GLTFScene> scene,
	VkCommandBuffer commandBuffer,
	VkDescriptorSet globalDescriptorSet,
	VkDescriptorSet lightsDescriptorSet,
	VkDescriptorSet materialsDescriptorSet
);

//the full renderer
class GraphicsRenderer {
	VulkanCore core;
	VkRect2D renderRegion;
	RC<GLTFScene> scene;
	RC<ForwardRenderer> forward;
	RC<PostProcessRenderer> postProcess;

	struct {
		GlobalFrameData globalFrameData;	//contains global data such as projection, view matrices
		VkDescriptorSet pointLightsDS;
	} perFrame[FRAMES_IN_FLIGHT];

	void initPerFrame() {
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto& frame = perFrame[i];
			
			frame.globalFrameData.init(core);
			
			frame.pointLightsDS = this->scene->lightsBuffer.createDescriptorSet(
				i,
				core,
				VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT
			);
		}
	}

	VkPipelineLayout forwardPipelineLayout;
	void createForwardPipelineLayout() {
		//setup push constants
		VkPushConstantRange push_constant;
		push_constant.offset = 0;
		push_constant.size = sizeof(MeshPushConstants);
		push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		std::array<VkDescriptorSetLayout, 3> layouts = {
			core->getLayout(perFrame[0].globalFrameData.descriptor),
			core->getLayout(perFrame[0].pointLightsDS),
			core->getLayout(scene->materialsDescriptorSet)
		};
		pipelineLayoutInfo.setLayoutCount = layouts.size();
		pipelineLayoutInfo.pSetLayouts = layouts.data();

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &push_constant;

		forwardPipelineLayout = core->createPipelineLayout(pipelineLayoutInfo);
	}

	struct {
		UniqueSampler renderSampler;
		UniqueSampler depthSampler;
		UniqueSampler normalSampler;
		void init(VulkanCore core) {
			renderSampler = Sampler::create(
				core,
				Sampler::makeCreateInfo(
					{ VK_FILTER_NEAREST, VK_FILTER_NEAREST }, {}, {}, {}, {}, {}, {}, {}
				)
			);
			depthSampler = Sampler::create(
				core,
				Sampler::makeCreateInfo(
					{ VK_FILTER_NEAREST, VK_FILTER_NEAREST }, {}, {}, {}, {}, {}, {}, {}
				)
			);
			normalSampler = Sampler::create(
				core,
				Sampler::makeCreateInfo(
					{ VK_FILTER_NEAREST, VK_FILTER_NEAREST }, {}, {}, {}, {}, {}, {}, {}
				)
			);
		}
	} postProcessSamplers;

	VkDescriptorSet postProcessDS;
	void createPostProcessDS() {
		VkDescriptorSetLayoutBinding binding0{}, binding1{}, binding2{};
		binding0.binding = 0;
		binding0.descriptorCount = 1;
		binding0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding0.pImmutableSamplers = nullptr;
		binding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		binding1 = binding0;
		binding1.binding = 1;

		binding2 = binding0;
		binding2.binding = 2;

		postProcessDS = core->createDescriptorSet({ binding0, binding1, binding2 });
	}

	struct {
		UniqueImageView renderImageView;
		UniqueImageView depthImageView;
		UniqueImageView normalImageView;
	} postProcessInputs;
	void setTexturesToPostProcessDS(
		std::shared_ptr<Image> renderImage,
		std::shared_ptr<Image> depthImage,
		std::shared_ptr<Image> normalImage
	) {
		VkDescriptorImageInfo imageInfoRender{}, imageInfoDepth{}, imageInfoNormal{};

		postProcessInputs.renderImageView = getImageView(
			renderImage,
			renderImage->format,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		imageInfoRender.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoRender.imageView = postProcessInputs.renderImageView->view;
		imageInfoRender.sampler = postProcessSamplers.renderSampler->sampler;

		postProcessInputs.depthImageView = getImageView(
			depthImage,
			depthImage->format,
			VK_IMAGE_ASPECT_DEPTH_BIT
		);
		imageInfoDepth.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoDepth.imageView = postProcessInputs.depthImageView->view;
		imageInfoDepth.sampler = postProcessSamplers.depthSampler->sampler;

		postProcessInputs.normalImageView = getImageView(
			normalImage,
			normalImage->format,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
		imageInfoNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfoNormal.imageView = postProcessInputs.normalImageView->view;
		imageInfoNormal.sampler = postProcessSamplers.normalSampler->sampler;

		VkWriteDescriptorSet write0{}, write1{}, write2{};

		write0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write0.descriptorCount = 1;
		write0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write0.dstSet = postProcessDS;
		write0.dstBinding = 0;
		write0.pNext = nullptr;
		write0.pImageInfo = &imageInfoRender;

		write1 = write0;
		write1.dstBinding = 1;
		write1.pImageInfo = &imageInfoDepth;

		write2 = write0;
		write2.dstBinding = 2;
		write2.pImageInfo = &imageInfoNormal;

		VkWriteDescriptorSet writes[3] = { write0, write1, write2 };

		vkUpdateDescriptorSets(core->device, 3, writes, 0, nullptr);
	}

	VkPipelineLayout postProcessPipelineLayout;
	void createPostProcessPipelineLayout() {
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = 1;
		VkDescriptorSetLayout postProcessDescLayout = core->getLayout(postProcessDS);
		pipelineLayoutInfo.pSetLayouts = &postProcessDescLayout;;

		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		postProcessPipelineLayout = core->createPipelineLayout(pipelineLayoutInfo);
	}

public:
	void init(VulkanCore coreIN, VkRect2D renderRegionIN, RC<GLTFScene> sceneIN){
		core = coreIN;
		renderRegion = renderRegionIN;
		scene = sceneIN;
		initPerFrame();

		createForwardPipelineLayout();
		forward = ForwardRenderer::create(core, renderRegion, forwardPipelineLayout);
		
		postProcessSamplers.init(core);
		createPostProcessDS();
		createPostProcessPipelineLayout();
		postProcess = PostProcessRenderer::create(core, renderRegion, postProcessPipelineLayout);

		setTexturesToPostProcessDS(
			forward->colorOutputImage,
			forward->depthImage,
			forward->gNormal
		);
	}

	void render(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
		{
			assert(frameIndex < FRAMES_IN_FLIGHT);
			forward->colorOutputImage->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			forward->depthImage->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
			forward->gNormal->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		forwardRenderScene(
			forward,
			scene,
			commandBuffer,
			perFrame[frameIndex].globalFrameData.descriptor,
			perFrame[frameIndex].pointLightsDS,
			scene->materialsDescriptorSet
		);

		//postprocessing
		{
			forward->colorOutputImage->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			forward->depthImage->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
			forward->gNormal->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			postProcess->colorOutputImage->cmdTransitionImageLayout(
				commandBuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			postProcess->beginRender(commandBuffer);

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				postProcess->getPipelineLayout(),
				0,
				1, &this->postProcessDS,
				0, nullptr
			);

			postProcess->endRender(commandBuffer);
		}
	}

	RC<Image> getResultImage() {
		return postProcess->colorOutputImage;
	}
};


void forwardRenderScene(
	RC<ForwardRenderer> forwardRenderer,
	std::shared_ptr<GLTFScene> scene,
	VkCommandBuffer commandBuffer,
	VkDescriptorSet globalDescriptorSet,
	VkDescriptorSet lightsDescriptorSet,
	VkDescriptorSet materialsDescriptorSet) {

	forwardRenderer->beginRender(commandBuffer);


	VkDescriptorSet descriptors[2] = { globalDescriptorSet, lightsDescriptorSet };

	VkPipelineLayout pipelineLayout = forwardRenderer->getPipelineLayout();

	vkCmdBindDescriptorSets(commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
		0, 2, descriptors,
		0, nullptr
	);

	scene->drawAll(
		commandBuffer,
		[pipelineLayout](VkCommandBuffer cmd, GLTFScene& scene, const GLTFDrawable& drawable) {
			uint32_t dynamicOffset = scene.materials.getResourceOffset(drawable.material);
			vkCmdBindDescriptorSets(
				cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout, 2, 1, &scene.materialsDescriptorSet, 1, &dynamicOffset
			);

			MeshPushConstants constants{ drawable.transform };
			vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
		}
	);

	forwardRenderer->endRender(commandBuffer);
}
