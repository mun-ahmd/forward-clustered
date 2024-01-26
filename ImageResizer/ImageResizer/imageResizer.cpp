#define _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "glm/glm.hpp"

#include <optional>
#include <memory>
#include <iostream>
#include <string>

typedef std::unique_ptr<unsigned char[], void(*)(void*)> ImagePtr;

ImagePtr loadImageFromFile(
	const char* filename,
	int* width,
	int* height,
	int* actualChannels,
	int desiredChannels
){
	return std::unique_ptr<unsigned char[], void(*)(void*)>(
		stbi_load(filename, width, height, actualChannels, desiredChannels),
		[](void* ptr) {
			stbi_image_free(ptr);
		}
	);
}

ImagePtr resizeImage(
	ImagePtr image,
	int numChannels,
	int oldWidth,
	int oldHeight,
	int newWidth,
	int newHeight,
	bool isSRGB
) {
	unsigned char* newImage = (unsigned char*)malloc(sizeof(unsigned char) * newWidth * newHeight * numChannels);
	constexpr stbir_pixel_layout pixelLayouts[4] = {
		STBIR_1CHANNEL,
		STBIR_2CHANNEL,
		STBIR_RGB,
		STBIR_RGBA
	};
	auto pixelLayout = pixelLayouts[numChannels - 1];

	if (isSRGB) {
		stbir_resize_uint8_srgb(image.get(), oldWidth, oldHeight, 0, newImage, newWidth, newHeight, 0, pixelLayout);
	}
	else {
		stbir_resize_uint8_linear(image.get(), oldWidth, oldHeight, 0, newImage, newWidth, newHeight, 0, pixelLayout);
	}

	return ImagePtr(
		newImage,
		[](void* ptr) {
			free(ptr);
		}
	);
}

ImagePtr loadImage(
    const char* filepath,
    int& iw,
    int& ih,
    int& ic,
    bool isSRGB,
    std::optional<glm::ivec2> desiredResolution = {}
) {
    //loads images from files and then
    //creates a VkImage trying its best to find the right format
    auto imageData = loadImageFromFile(
        filepath,
        &iw,
        &ih,
        &ic,
        0);

    std::cout << "Image: " << filepath << " was " << iw << "x" << ih << " pixels" << std::endl;

    if (desiredResolution.has_value() && glm::ivec2(iw, ih) != desiredResolution.value()) {
        //need to resize image
        imageData = resizeImage(
            std::move(imageData),
            ic,
            iw,
            ih,
            desiredResolution->x,
            desiredResolution->y,
            true
        );

        iw = desiredResolution->x;
        ih = desiredResolution->y;
    }
    return imageData;
}

void saveImage(const char* filepath, ImagePtr image, int x, int y, int components){
    stbi_write_png(filepath, x, y, components, image.get(), 0);
}

#include <filesystem>
#include <vector>
namespace fs = std::filesystem;

int main(int argc, char** argv){
    if (argc == 2) {
        std::string ask(argv[1]);
        if (ask == "-h" || ask == "--help") {
            std::cout << "Image Resizer\nusage:\n\tImageResizer <source directory> <destination directory> --options\n";
            std::cout << "options:\n\t-f --format (extension) add a image format to resize\n";
            std::cout << "\t-x --xSize (number) target x resolution\n";
            std::cout << "\t-y --ySize (number) target y resolution\n";
            std::cout << "\t-s --srgb (true/false) srgb correct resize (default true)\n";
            std::cout << "example usage\n\tImageResizer ./Source/ ./Target/ -f .jpg -f .png -f .bmp -x 256 -y 128 -s false";
            std::cout << std::endl;
            return 0;
        }
    }

    if(argc < 3){
        std::cerr << "Error: need source/target directory to find/store textures" << std::endl;
        return 1;
    }
    
    //empty
    fs::path sourceDir = "";
    fs::path targetDir = "";

    bool switched = false;
    std::string lastSwitch = "";

    std::vector<std::string> acceptedFormats;
    std::optional<glm::ivec2> desiredResizeRes = glm::ivec2(-1, -1);

    bool isSRGB = true;

    for(int arg = 1; arg < argc; arg++){
        std::string curr = argv[arg];
        if(!switched){
            if(curr[0] == '-'){
                switched = true;
                lastSwitch = curr;
            }
            else if(sourceDir.empty()){
                try {
                    sourceDir = fs::path(curr);
                    if (fs::is_directory(sourceDir) == false) {
                        throw std::exception("path is not a directory");
                    }
                }
                catch (std::exception e) {
                    std::cerr << "Error: invalid source directory" << std::endl;
                    std::cerr << e.what() << std::endl;
                }
            }
            else if (targetDir.empty()) {
                try {
                    targetDir = fs::path(curr);
                    if (fs::is_directory(targetDir) == false) {
                        throw std::exception("path is not a directory");
                    }
                }
                catch (std::exception e) {
                    std::cerr << "Error: invalid target directory" << std::endl;
                    std::cerr << e.what() << std::endl;
                }
            }
            else{
                std::cerr << "Error: invalid parameter at position " << arg << " " << curr << std::endl;
                return 2;
            }
        }
        else{
            switched = false;
            if(lastSwitch == "-f" || lastSwitch == "--format"){
                acceptedFormats.push_back(curr);
            }
            else if(lastSwitch == "-x" || lastSwitch =="--xSize"){
                try{
                    desiredResizeRes->x = std::stoi(curr);
                }
                catch(std::exception& e){
                    std::cerr << "Error: invalid parameter at position " << arg << " " << curr << std::endl;
                    std::cerr << e.what() << std::endl;
                    return 3;
                }
            }
            else if(lastSwitch == "-y" || lastSwitch =="--ySize"){
                try{
                    desiredResizeRes->y = std::stoi(curr);
                }
                catch(std::exception& e){
                    std::cerr << "Error: invalid parameter at position " << arg << " " << curr << std::endl;
                    std::cerr << e.what() << std::endl;
                    return 3;
                }
            }
            else if(lastSwitch == "-s" || lastSwitch == "--srgb"){
                if(curr == "true"){
                    isSRGB = true;
                }
                else if(curr == "false"){
                    isSRGB = false;
                }
                else{
                    std::cerr << "Error: invalid parameter at position " << arg << " " << curr << std::endl;
                    return 4;
                }
            }
        }
    }

    if (sourceDir.empty() || targetDir.empty()) {
        std::cerr << "missing source/target directory" << std::endl;
        return 10;
    }

    if(acceptedFormats.empty()){
        acceptedFormats = {".jpg", ".jpeg", ".png", ".bmp"};
    }

    if(desiredResizeRes.value() == glm::ivec2(-1, -1)){
        desiredResizeRes = std::nullopt;
    }
    else if(desiredResizeRes->x < 1 || desiredResizeRes->y < 1){
        std::cerr << "Invalid resize resolution" << std::endl;
        return 5;
    }
    
    std::cout << "DEBUG INFO:\n";
    std::cout << sourceDir << std::endl;
    for(auto f : acceptedFormats){
        std::cout << f << std::endl;
    }
    auto xxdes = desiredResizeRes.value_or(glm::ivec2(-1, -1));
    std::cout << desiredResizeRes.has_value() << " " << xxdes.x << " "  << xxdes.y << std::endl;
    std::cout << (isSRGB ? "srgb true" : "srgb false") << std::endl;


    for (const auto& entry : fs::directory_iterator(sourceDir)) {
        if (fs::is_regular_file(entry)){
            auto extension = entry.path().extension().generic_string();
            bool isFormatAccepted = false;
            for(std::string& format : acceptedFormats){
                if(extension == format){
                    isFormatAccepted = true;
                    break;
                }
            }
            
            if(!isFormatAccepted)
                continue;

            std::string imagePath = entry.path().generic_string();
            int iw, ih, ic;
            ImagePtr image = loadImage(imagePath.c_str(), iw, ih, ic, isSRGB, desiredResizeRes);
            saveImage((targetDir / entry.path().filename()).generic_string().c_str(), std::move(image), iw, ih, ic);
        }
    }
    
    return 0;
}