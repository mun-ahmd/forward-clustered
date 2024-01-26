#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>

class ImageData {
private:
	ImageData(unsigned char* data, int width, int height, int numComponents, bool isSRGB);

	unsigned char* data;
	int width;
	int height;
	int numComponents;
	bool isImageSRGB;

public:
	ImageData(const char* filepath, bool isSRGB, int desiredComponents);
	ImageData(const ImageData&) = delete;
	~ImageData();

	unsigned char* getData();
	int getWidth();
	int getHeight();
	int getNumComponents();
	bool isSRGB();
	
	glm::ivec2 getResolution();
	size_t getSizeInBytes();

	std::unique_ptr<ImageData> resize(int newWidth, int newHeight);
};

typedef std::unique_ptr<ImageData> ImagePtr;
ImagePtr loadImageFromFile(const char* filepath, bool isSRGB, int desiredComponents);