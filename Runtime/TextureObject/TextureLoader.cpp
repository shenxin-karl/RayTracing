#include "TextureLoader.h"
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
#include "Renderer/GfxDevice.h"

auto TextureLoader::LoadFromFile(stdfs::path path, bool forceSRGB) -> SharedPtr<dx::Texture> {
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
        pImageLoader = std::make_unique<FileDDSLoader>();
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
    SharedPtr<dx::Texture> pTexture = UploadTexture(pImageLoader.get(), forceSRGB);
    pTexture->SetName(path.string());
    _textureMap[path] = pTexture;
    return pTexture;
}

auto TextureLoader::GetSRV2D(const dx::Texture *pTexture) -> dx::SRV {
    auto iter = _srv2DMap.find(pTexture);
    if (iter != _srv2DMap.end()) {
	    return iter->second;
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::Device *pDevice = pGfxDevice->GetDevice();

    dx::SRV srv = pDevice->AllocDescriptor<dx::SRV>(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = pTexture->GetFormat();
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = pTexture->GetMipCount();
    desc.Texture2D.PlaneSlice = 0;
    desc.Texture2D.ResourceMinLODClamp = 0.f;
    pDevice->GetNativeDevice()->CreateShaderResourceView(pTexture->GetResource(), &desc, srv.GetCpuHandle());
    _srv2DMap.emplace_hint(iter, std::make_pair(pTexture, srv));
    return srv;
}

auto TextureLoader::GetSRVCube(const dx::Texture *pTexture) -> dx::SRV {
    Assert(pTexture->GetDepthOrArraySize() == 6);
    auto iter = _srvCubeMap.find(pTexture);
    if (iter != _srvCubeMap.end()) {
	    return iter->second;
    }

    GfxDevice *pGfxDevice = GfxDevice::GetInstance();
    dx::NativeDevice *device= pGfxDevice->GetDevice()->GetNativeDevice();

    dx::SRV srv = pGfxDevice->GetDevice()->AllocDescriptor<dx::SRV>(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    view.Format = pTexture->GetFormat();
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.TextureCube.MostDetailedMip = 0;
    view.TextureCube.MipLevels = pTexture->GetMipCount();
    view.TextureCube.ResourceMinLODClamp = 0.f;
    device->CreateShaderResourceView(pTexture->GetResource(), &view, srv.GetCpuHandle());
    _srvCubeMap.emplace_hint(iter, std::make_pair(pTexture, srv));
    return srv;
}

auto TextureLoader::UploadTexture(dx::IImageLoader *pLoader, bool forceSRGB) -> SharedPtr<dx::Texture> {
	dx::UploadHeap *pUploadHeap = GfxDevice::GetInstance()->GetUploadHeap();

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

    if (forceSRGB) {
		textureDesc.Format = dx::GetSRGBFormat(textureDesc.Format);    
    }
    SharedPtr<dx::Texture> pTexture = dx::Texture::Create(pDevice, textureDesc, D3D12_RESOURCE_STATE_COPY_DEST);

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
