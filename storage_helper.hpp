#pragma once
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

class Store {
private:
	inline static fs::path directory = fs::path("ObjectStore");

public:
	inline static fs::path getStorePath(const char* name) {
		return (directory / name).replace_extension(".bin");
	}

	inline static bool itemInStore(const char* name) {
		return fs::is_regular_file(getStorePath(name));
	}

	void static storeBytes(const char* name, const char* bytes, unsigned int size) {
		std::ofstream file(
			getStorePath(name),
			std::ios::binary
		);
		
		if (!file)
			throw std::runtime_error("Failed to open file for writing");

		file.write(bytes, size);
	}

	static std::unique_ptr<char[]> fetchBytes(const char* name, unsigned int* loadedSize = nullptr) {
		assert(itemInStore(name));
		
		std::ifstream file(
			getStorePath(name),
			std::ios::binary
		);

		if (!file)
			throw std::runtime_error("Failed to open file for reading");

		// Determine the file size
		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::unique_ptr<char[]> bytes = std::make_unique<char[]>(size);
		if (!file.read(bytes.get(), size))
			throw std::runtime_error("Failed to read from file");

		if (loadedSize) {
			*loadedSize = size;
		}

		return bytes;
	}
};