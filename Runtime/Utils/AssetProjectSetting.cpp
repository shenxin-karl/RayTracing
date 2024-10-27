#include "AssetProjectSetting.h"
#include "Foundation/Exception.h"

IMPLEMENT_SERIALIZER(AssetProjectSetting)

template<TransferContextConcept Archive>
void AssetProjectSetting::TransferImpl(Archive &transfer) {
    TRANSFER_VERSION("AssetProjectSetting", 1);
    TRANSFER(_assetRelativePath);
    TRANSFER(_assetAbsolutePath);
    TRANSFER(_assetCacheRelativePath);
    TRANSFER(_assetCacheAbsolutePath);
}

void AssetProjectSetting::OnCreate() {
    TransferJsonReader jsonReader(sSerializePath);
    if (!Serialize(jsonReader, *this)) {
        _assetRelativePath = "./Assets";
        _assetCacheRelativePath = "./AssetsCache";
    }

    RepairPath(_assetRelativePath, _assetAbsolutePath);
    RepairPath(_assetCacheRelativePath, _assetCacheAbsolutePath);
}

void AssetProjectSetting::OnDestroy() {
    Exception::CondThrow(SerializeToFile(), "Serialize Asset Project Setting failed!");
}

void AssetProjectSetting::SetAssetRelativePath(stdfs::path path) {
    _assetRelativePath = std::move(path);
    RepairPath(_assetRelativePath, _assetAbsolutePath);
}

void AssetProjectSetting::SetAssetAbsolutePath(stdfs::path path) {
    _assetAbsolutePath = std::move(path);
    RepairPath(_assetRelativePath, _assetAbsolutePath);
}

void AssetProjectSetting::SetAssetCacheRelativePath(stdfs::path path) {
    _assetCacheRelativePath = std::move(path);
    RepairPath(_assetCacheRelativePath, _assetCacheAbsolutePath);
}

void AssetProjectSetting::SetAssetCacheAbsolutePath(stdfs::path path) {
    _assetCacheAbsolutePath = std::move(path);
    RepairPath(_assetCacheRelativePath, _assetCacheAbsolutePath);
}

bool AssetProjectSetting::SerializeToFile() {
    TransferJsonWriter jsonWriter(sSerializePath);
    return Serialize(jsonWriter, *this);
}

void AssetProjectSetting::RepairPath(stdfs::path &relativePath, stdfs::path &absolutePath) {
    if (!relativePath.empty() && !relativePath.is_relative()) {
        relativePath = relative(relativePath);
    }
    if (!absolutePath.empty() && !absolutePath.is_absolute()) {
        absolutePath = absolute(absolutePath);
    }

    if (!relativePath.empty() && absolutePath.empty()) {
        absolutePath = stdfs::absolute(relativePath);
    } else if (relativePath.empty() && !absolutePath.empty()) {
        relativePath = stdfs::relative(absolutePath);
    }

    if (!stdfs::exists(absolutePath)) {
        stdfs::create_directory(absolutePath);
    }
}
