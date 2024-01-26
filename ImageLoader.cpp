#include "ImageLoader.hpp"
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

ImageData::ImageData(
	unsigned char* data,
	int width,
	int height,
	int numComponents,
	bool isSRGB
) : data(data), width(width), height(height), numComponents(numComponents), isImageSRGB(isSRGB) {}

ImageData::ImageData(const char* filepath, bool isSRGB, int desiredComponents) : isImageSRGB(isSRGB) {
	this->data = stbi_load(
		filepath,
		&width,
		&height,
		&numComponents,
		desiredComponents
	);
	if (desiredComponents != 0)
		this->numComponents = desiredComponents;

	if (this->data == NULL) {
		throw std::runtime_error("Image: " + std::string(filepath) + " could not be loaded successfully");
	}
}

ImageData::~ImageData() {
	STBI_FREE(this->data);
}

constexpr stbir_pixel_layout pixelLayouts[4] = {
	STBIR_1CHANNEL,
	STBIR_2CHANNEL,
	STBIR_RGB,
	STBIR_RGBA
};

std::unique_ptr<ImageData> ImageData::resize(int newWidth, int newHeight) {
	stbir_pixel_layout pixelLayout = pixelLayouts[numComponents - 1];
	unsigned char* newImageData = (unsigned char*)STBI_MALLOC(sizeof(unsigned char) * newWidth * newHeight * numComponents);
	if (isImageSRGB) {
		stbir_resize_uint8_srgb(data, width, height, 0, newImageData, newWidth, newHeight, 0, pixelLayout);
	}
	else {
		stbir_resize_uint8_linear(data, width, height, 0, newImageData, newWidth, newHeight, 0, pixelLayout);
	}

	ImageData* imageData = new ImageData(
		newImageData,
		newWidth,
		newHeight,
		numComponents,
		isImageSRGB
	);

	return ImagePtr(imageData);
}

unsigned char* ImageData::getData() {
	return this->data;
}

int ImageData::getWidth() {
	return this->width;
}

int ImageData::getHeight() {
	return this->height;
}

int ImageData::getNumComponents() {
	return this->numComponents;
}

bool ImageData::isSRGB() {
	return this->isImageSRGB;
}

glm::ivec2 ImageData::getResolution() {
	return glm::ivec2(
		this->width,
		this->height
	);
}

size_t ImageData::getSizeInBytes() {
	assert(width > 0 && height > 0 && numComponents > 0);
	return (size_t)this->height * this->width * this->numComponents;
}

ImagePtr loadImageFromFile(const char* filepath, bool isSRGB, int desiredComponents) {
	return std::make_unique<ImageData>(filepath, isSRGB, desiredComponents);
}