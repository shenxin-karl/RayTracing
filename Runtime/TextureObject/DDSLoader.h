#pragma once
#include <dxgiformat.h>
#include <fstream>
#include "D3d12/IImageLoader.h"

class FileDDSLoader : public dx::IFileImageLoader {
public:
	FileDDSLoader();
	~FileDDSLoader() override;
public:
	bool Load(const stdfs::path &filePath, float cutOff) override;
	auto GetImageHeader() const -> dx::ImageHeader override;
	void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
	// clang-format off
	std::ifstream		_file;
    dx::ImageHeader     _imageHeader;
	// clang-format on
};

class MemoryDDSLoader : public dx::IMemoryImageLoader {
public:
	MemoryDDSLoader();
	~MemoryDDSLoader() override;
public:
	bool Load(const uint8_t *pData, size_t dataSize, float cutOff) override;
	auto GetImageHeader() const -> dx::ImageHeader override;
	void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
	// clang-format off
	const uint8_t	   *_pData;
	const uint8_t	   *_pCurrent;
	size_t				_dataSize;
    dx::ImageHeader     _imageHeader;
	// clang-format on
};