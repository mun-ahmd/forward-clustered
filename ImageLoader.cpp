#include "ImageLoader.hpp"
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize2.h"

constexpr stbir_pixel_layout pixelLayouts[4] = {
	STBIR_1CHANNEL,
	STBIR_2CHANNEL,
	STBIR_RGB,
	STBIR_RGBA
};
constexpr stbir_pixel_layout numComponentsToLayout(uint8_t numComponents) {
	return pixelLayouts[numComponents - 1];
}


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

std::unique_ptr<ImageData> ImageData::resize(int newWidth, int newHeight) {
	stbir_pixel_layout pixelLayout = numComponentsToLayout(numComponents);
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


FloatImageData::FloatImageData(
	float* data,
	int width,
	int height,
	int numComponents
) : data(data), width(width), height(height), numComponents(numComponents) {}

FloatImageData::FloatImageData(const char* filepath, int desiredComponents) {
	this->data = stbi_loadf(
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

FloatImageData::~FloatImageData() {
	STBI_FREE(this->data);
}

std::unique_ptr<FloatImageData> FloatImageData::resize(int newWidth, int newHeight) {
	stbir_pixel_layout pixelLayout = numComponentsToLayout(numComponents);
	
	float* newImageData = (float*)STBI_MALLOC(sizeof(unsigned char) * newWidth * newHeight * numComponents);
	
	stbir_resize_float_linear(data, width, height, 0, newImageData, newWidth, newHeight, 0, pixelLayout);
	
	FloatImageData* imageData = new FloatImageData(
		newImageData,
		newWidth,
		newHeight,
		numComponents
	);

	return FloatImagePtr(imageData);
}

float* FloatImageData::getData() {
	return this->data;
}

int FloatImageData::getWidth() {
	return this->width;
}

int FloatImageData::getHeight() {
	return this->height;
}

int FloatImageData::getNumComponents() {
	return this->numComponents;
}

glm::ivec2 FloatImageData::getResolution() {
	return glm::ivec2(
		this->width,
		this->height
	);
}

size_t FloatImageData::getSizeInBytes() {
	assert(width > 0 && height > 0 && numComponents > 0);
	return (size_t)this->height * this->width * this->numComponents * sizeof(float);
}

FloatImagePtr loadFloatImageFromFile(const char* filepath, int desiredComponents) {
	return std::make_unique<FloatImageData>(filepath, desiredComponents);
}
