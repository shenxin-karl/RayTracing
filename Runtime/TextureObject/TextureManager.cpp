#include "TextureManager.h"
#include "DDSLoader.h"
#include "WICLoader.h"
#include "Foundation/PathUtils.h"
#include "Foundation/StringUtil.h"
#include "Utils/AssetProjectSetting.h"
#include <d3d12.h>
#include "D3d12/d3dx12.h"
#include "D3d12/Device.h"
#include "D3d12/Texture.h"
#include "D3d12/UploadHeap.h"
#include "Foundation/Formatter.hpp"
#include <ranges>

void TextureManager::OnCreate() {
}

void TextureManager::OnDestroy() {
    for (std::shared_ptr<dx::Texture> &pTexture : _textureMap | std::views::values) {
	    pTexture->OnDestroy();
    }
    _textureMap.clear();
}

auto TextureManager::LoadFromFile(stdfs::path path, dx::UploadHeap *pUploadHeap, bool makeSRGB) -> std::shared_ptr<dx::Texture> {
    if (!path.is_absolute()) {
        path = stdfs::absolute(path);
    }

    if (auto iter = _textureMap.find(path); iter != _textureMap.end()) {
        return iter->second;
    }

    if (!stdfs::exists(path)) {
        Exception::Throw("the file '{}'  does not exist", path);
    }

    const stdfs::path &projectPath = AssetProjectSetting::GetInstance()->GetAssetAbsolutePath();
    if (!nstd::IsSubPath(projectPath, path)) {
        Exception::Throw("'path' must be under the project path");
    }

    std::unique_ptr<dx::IFileImageLoader> pImageLoader;
    std::string extension = nstd::tolower(path.extension().string());
    if (extension == ".dds") {
        pImageLoader = std::make_unique<DDSLoader>();
    } else {
        std::string_view supportExtensions[] = {".jpg", ".png", ".tga", ".bmp", ".psd", ".hdr", ".pic"};
        for (std::string_view targetExtension : supportExtensions) {
            if (targetExtension == extension) {
                pImageLoader = std::make_unique<WICLoader>();
                break;
            }
        }
    }

    if (pImageLoader == nullptr) {
        Exception::Throw("Unsupported extended file formats: {}", extension);
    }

    pImageLoader->Load(path, 1.f);
    std::shared_ptr<dx::Texture> pTexture = UploadTexture(pImageLoader.get(), pUploadHeap, makeSRGB);
    pTexture->SetName(path.string());
    _textureMap[path] = pTexture;
    return pTexture;
}

auto TextureManager::UploadTexture(dx::IImageLoader *pLoader, dx::UploadHeap *pUploadHeap, bool makeSRGB)
    -> std::shared_ptr<dx::Texture> {

    std::shared_ptr<dx::Texture> pTexture = std::make_shared<dx::Texture>();
    dx::ImageHeader imageHeader = pLoader->GetImageHeader();

    dx::Device *pDevice = pUploadHeap->GetDevice();
    CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(imageHeader.format,
        imageHeader.width,
        imageHeader.height,
        imageHeader.arraySize,
        imageHeader.mipMapCount,
        1,
        0,
        D3D12_RESOURCE_FLAG_NONE);

    if (makeSRGB) {
		textureDesc.Format = dx::GetSRGBFormat(textureDesc.Format);    
    }
    pTexture->OnCreate(pDevice, textureDesc, D3D12_RESOURCE_STATE_COPY_DEST);

    UINT64 UplHeapSize;
    uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = {0};
    UINT64 row_sizes_in_bytes[D3D12_REQ_MIP_LEVELS] = {0};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex2D[D3D12_REQ_MIP_LEVELS];
    pDevice->GetNativeDevice()->GetCopyableFootprints(&textureDesc,
        0,
        imageHeader.mipMapCount,
        0,
        placedTex2D,
        num_rows,
        row_sizes_in_bytes,
        &UplHeapSize);

    UINT32 bytePP = dx::GetPixelByteSize(imageHeader.format);
    UINT32 pixelsPerBlock = 1;
    if (dx::IsBCFormat(imageHeader.format)) {
        pixelsPerBlock = (4 * 4);
        pixelsPerBlock /= 4;
    }

    for (uint32_t index = 0; index < imageHeader.arraySize; index++) {
        UINT8 *pixels = pUploadHeap->AllocBuffer(UplHeapSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        for (uint32_t mip = 0; mip < imageHeader.mipMapCount; mip++) {
            pLoader->GetNextMipMapData(pixels + placedTex2D[mip].Offset,
                placedTex2D[mip].Footprint.RowPitch,
                (placedTex2D[mip].Footprint.Width * bytePP) / pixelsPerBlock,
                num_rows[mip]);
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT slice = placedTex2D[mip];
            slice.Offset += (pixels - pUploadHeap->GetBasePtr());

            CD3DX12_TEXTURE_COPY_LOCATION dst(pTexture->GetResource(), index * imageHeader.mipMapCount + mip);
            CD3DX12_TEXTURE_COPY_LOCATION src(pUploadHeap->GetResource(), slice);
            pUploadHeap->AddTextureCopy({ src, dst });
        }
    }

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
	    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    UINT subResources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pUploadHeap->AddPostUploadTranslation(pTexture->GetResource(), state, subResources);

    return pTexture;
}
