#include "DDSLoader.h"
#include "Foundation/StreamUtil.h"
#include "D3d12/FormatHelper.hpp"

struct DDS_PIXELFORMAT {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t bitCount;
    uint32_t bitMaskR;
    uint32_t bitMaskG;
    uint32_t bitMaskB;
    uint32_t bitMaskA;
};

struct DDS_HEADER {

    uint32_t dwSize;
    uint32_t dwHeaderFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwSurfaceFlags;
    uint32_t dwCubemapFlags;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

enum RESOURCE_DIMENSION {
    RESOURCE_DIMENSION_UNKNOWN = 0,
    RESOURCE_DIMENSION_BUFFER = 1,
    RESOURCE_DIMENSION_TEXTURE1D = 2,
    RESOURCE_DIMENSION_TEXTURE2D = 3,
    RESOURCE_DIMENSION_TEXTURE3D = 4
};

struct DDS_HEADER_DXT10 {
    DXGI_FORMAT dxgiFormat;
    RESOURCE_DIMENSION resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t reserved;
};

static DXGI_FORMAT GetDxgiFormat(DDS_PIXELFORMAT pixelFmt) {
    if (pixelFmt.flags & 0x00000004)    //DDPF_FOURCC
    {
        // Check for D3DFORMAT enums being set here
        switch (pixelFmt.fourCC) {
        case '1TXD':
            return DXGI_FORMAT_BC1_UNORM;
        case '3TXD':
            return DXGI_FORMAT_BC2_UNORM;
        case '5TXD':
            return DXGI_FORMAT_BC3_UNORM;
        case 'U4CB':
            return DXGI_FORMAT_BC4_UNORM;
        case 'A4CB':
            return DXGI_FORMAT_BC4_SNORM;
        case '2ITA':
            return DXGI_FORMAT_BC5_UNORM;
        case 'S5CB':
            return DXGI_FORMAT_BC5_SNORM;
        case 'GBGR':
            return DXGI_FORMAT_R8G8_B8G8_UNORM;
        case 'BGRG':
            return DXGI_FORMAT_G8R8_G8B8_UNORM;
        case 36:
            return DXGI_FORMAT_R16G16B16A16_UNORM;
        case 110:
            return DXGI_FORMAT_R16G16B16A16_SNORM;
        case 111:
            return DXGI_FORMAT_R16_FLOAT;
        case 112:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 113:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case 114:
            return DXGI_FORMAT_R32_FLOAT;
        case 115:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 116:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        }
    } else {
        switch (pixelFmt.bitMaskR) {
        case 0xff:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case 0x00ff0000:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case 0xffff:
            return DXGI_FORMAT_R16G16_UNORM;
        case 0x3ff:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case 0x7c00:
            return DXGI_FORMAT_B5G5R5A1_UNORM;
        case 0xf800:
            return DXGI_FORMAT_B5G6R5_UNORM;
        case 0:
            return DXGI_FORMAT_A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        };
    }
}

DDSLoader::DDSLoader() {
}

DDSLoader::~DDSLoader() {
    _file.close();
}

bool DDSLoader::Load(const stdfs::path &filePath, float cutOff, dx::ImageHeader &imageHeader) {
    if (!stdfs::exists(filePath)) {
        return false;
    }

    _file.open(filePath);
    if (!_file.is_open()) {
        return false;
    }

    std::size_t fileSize = nstd::GetFileSize(_file);
    uint32_t rawTextureSize = fileSize;

    char headerData[4 + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10)];
    if (_file.read(headerData, sizeof(headerData))) {
        char *pByteData = headerData;
        uint32_t dwMagic = *reinterpret_cast<uint32_t *>(pByteData);
        if (dwMagic != ' SDD') {
            return false;
        }

        pByteData += 4;
        rawTextureSize -= 4;

        DDS_HEADER *header = reinterpret_cast<DDS_HEADER *>(pByteData);
        pByteData += sizeof(DDS_HEADER);
        rawTextureSize -= sizeof(DDS_HEADER);

        imageHeader.width = header->dwWidth;
        imageHeader.height = header->dwHeight;
        imageHeader.depth = header->dwDepth ? header->dwDepth : 1;
        imageHeader.mipMapCount = header->dwMipMapCount ? header->dwMipMapCount : 1;

        if (header->ddspf.fourCC == '01XD') {
            DDS_HEADER_DXT10 *header10 = reinterpret_cast<DDS_HEADER_DXT10 *>((char *)header + sizeof(DDS_HEADER));
            rawTextureSize -= sizeof(DDS_HEADER_DXT10);

            imageHeader.arraySize = header10->arraySize;
            imageHeader.format = header10->dxgiFormat;
            imageHeader.bitCount = header->ddspf.bitCount;
        } else {
            imageHeader.arraySize = (header->dwCubemapFlags == 0xfe00) ? 6 : 1;
            imageHeader.format = GetDxgiFormat(header->ddspf);
            imageHeader.bitCount = static_cast<uint32_t>(dx::BitsPerPixel(imageHeader.format));
        }
    }

    _file.seekg(std::ifstream::beg);
    return true;
}

void DDSLoader::GetNextMipMapData(void *pDest, uint32_t stride, uint32_t width, uint32_t height) {
    for (uint32_t y = 0; y < height; y++) {
        _file.read(static_cast<char*>(pDest) + y*stride, width);
    }
}
