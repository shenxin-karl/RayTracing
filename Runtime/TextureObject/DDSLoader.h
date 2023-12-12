#pragma once
#include <dxgiformat.h>
#include <fstream>
#include "D3d12/IImageLoader.h"

class DDSLoader : public dx::IFileImageLoader {
public:
	DDSLoader();
	~DDSLoader() override;
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
