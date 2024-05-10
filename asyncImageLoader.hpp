#pragma once
#include "ImageLoader.hpp"
#include "ConcurrentQueue.hpp"
#include "buffer.hpp"

#include <glm/glm.hpp>

#include <thread>
#include <atomic>

#include <string>
#include <iostream>
#include <vector>
#include <functional>

//todo add multiple workers

struct ImageLoadRequest{
	std::string path;
	glm::ivec3 resolution;
	int desiredChannels;

	bool isSRGB = true;
	bool isFloat = false;

	std::function<void(ImageLoadRequest&, std::shared_ptr<Buffer>)> callback;

	bool __isComplete;
	bool __isSuccess;
	
	int __assigned_buffer__; //internal do not use
	int __id__; //internal
};

class AsyncImageLoader {
private:
	const glm::ivec3 maxDimensions;
	const int maxNumComponents;
	const size_t bufferSizeBytes;
	const int numBuffers;

	std::vector<std::shared_ptr<Buffer>> stagingBuffers;
	//boolean telling whether it is free or not (if free it is true)
	std::vector<bool> isBufferFree;

	ConcurrentQueue<ImageLoadRequest> requestQueue;
	ConcurrentQueue<ImageLoadRequest> resultQueue;

	std::thread worker;
	std::atomic<bool> stopWorker{ false };
	bool isWorkerRunning = false;
	

	void workerThread() {
		while (!stopWorker) {
			if (requestQueue.empty())
				continue;

			//allot staging buffer to request
			int allotted = 0;
			{
				for (allotted; allotted < this->stagingBuffers.size(); allotted++) {
					if (this->isBufferFree[allotted]) {
						break;
					}
				}
				if (allotted == stagingBuffers.size()) {
					//could not allot
					continue;
				}
				//mark not free anymore
				this->isBufferFree[allotted] = false;
			}

			auto req = requestQueue.pop();
			std::cout << "Worker " << req.__id__ << " popped: " << req.path << " " << req.resolution.x << "x" << req.resolution.y  << " (" << req.desiredChannels << ")" << std::endl;

			std::shared_ptr<Buffer> staging = this->stagingBuffers[req.__assigned_buffer__];

			{
				if (req.isFloat) {
					FloatImagePtr imageData = loadFloatImageFromFile(
						req.path.c_str(),
						req.desiredChannels
					);

					if (imageData->getResolution() != glm::ivec2(req.resolution)) {
						//need to resize image
						imageData = std::move(imageData->resize(req.resolution.x, req.resolution.y));
					}

					memcpy(
						staging->getMappedData(),
						imageData->getData(),
						imageData->getSizeInBytes()
					);
					imageData.reset();
				}
				else
				{
					ImagePtr imageData = loadImageFromFile(
						req.path.c_str(),
						req.isSRGB,
						req.desiredChannels
					);

					if (imageData->getResolution() != glm::ivec2(req.resolution)) {
						//need to resize image
						imageData = std::move(imageData->resize(req.resolution.x, req.resolution.y));
					}

					memcpy(
						staging->getMappedData(),
						imageData->getData(),
						imageData->getSizeInBytes()
					);

					imageData.reset();
				}
			}

			req.__isComplete = true;
			req.__isSuccess = true;
			resultQueue.push(req);
		}
	}

public:
	AsyncImageLoader() = delete;
	AsyncImageLoader(const AsyncImageLoader& loader) = delete;

	//maxDimensions apply for float images, so normal byte per component images have a larger upper bound
	AsyncImageLoader(glm::ivec3 maxDimensions, int maxNumComponents, int numBuffers)
		: maxDimensions(maxDimensions), maxNumComponents(maxNumComponents), numBuffers(numBuffers),
		bufferSizeBytes((size_t)maxDimensions.x * maxDimensions.y * maxDimensions.z * maxNumComponents * sizeof(float))
	{}

	~AsyncImageLoader() {
		if (isWorkerRunning) {
			this->stop();
		}
		if (!this->requestQueue.empty()) {
			std::cout << "Warning Async Image Loader being destroyed with non empty requests queue" << std::endl;
		}
		if (!this->resultQueue.empty()) {
			std::cout << "Warning Async Image Loader being destroyed with non empty results queue" << std::endl;
		}
	}
	
	void init() {
		//create staging buffers
		size_t maxSize = bufferSizeBytes;

		for (int i = 0; i < this->numBuffers; i++) {
			this->stagingBuffers.push_back(
				prepareStagingBufferPersistant(VulkanUtils::utils().getCore(), maxSize)
			);
			this->isBufferFree.push_back(true);
		}
	}

	void start() {
		assert(isWorkerRunning == false);
		
		stopWorker = false;
 		this->worker = std::move(std::thread(
			&AsyncImageLoader::workerThread,
			this
		));
		isWorkerRunning = true;
	}

	void stop() {
		assert(isWorkerRunning == true);
		
		stopWorker = true;
		worker.join();
		isWorkerRunning = false;
	}

	int ids = 0;
	void request(ImageLoadRequest req) {
#ifndef NDEBUG
		if (
			(
				(size_t)req.resolution.x *
				req.resolution.y *
				req.resolution.z *
				(req.isFloat ? sizeof(float) : sizeof(unsigned char))
			) > bufferSizeBytes
		){
			std::cerr << (
				"ImageLoadRequest resolution too large! " +
				req.path + ": " +
				std::to_string(req.resolution.x) + "x" +
				std::to_string(req.resolution.y) + "x" +
				std::to_string(req.resolution.z)
				);
			assert(false);
		}
#endif

		req.__id__ = ids++;
		req.__isComplete = false;
		req.__isSuccess = false;
		
		this->requestQueue.push(req);
	}

	void processResults() {
		while (!resultQueue.empty()) {
			auto result = resultQueue.pop();
			if (result.__isComplete && result.__isSuccess) {
				result.callback(result, this->stagingBuffers[result.__assigned_buffer__]);
			}
			else {
				std::cerr << "Could not service async image load request" << std::endl;
			}
			this->isBufferFree[result.__assigned_buffer__] = true;
		}
	}
};