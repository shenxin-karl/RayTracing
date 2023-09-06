#pragma once
#include <dxgiformat.h>
#include <fstream>
#include "D3d12/IImageLoader.h"

class DDSLoader : public dx::IImageLoader {
public:
	DDSLoader();
	~DDSLoader() override;
public:
	bool Load(const stdfs::path &filePath, float cutOff, dx::ImageHeader &imageHeader) override;
	void GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) override;
private:
	std::ifstream _file;
};
