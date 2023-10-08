#pragma once
#include "Foundation/NamespeceAlias.h"
#include "Foundation/Singleton.hpp"
#include "Foundation/TypeBase.hpp"
#include "Serialize/Transfer.hpp"

class AssetProjectSetting
    : public TypeBase
    , public Singleton<AssetProjectSetting> {

    DECLARE_CLASS(AssetProjectSetting)
    DECLARE_SERIALIZER(AssetProjectSetting)
public:
    void OnCreate();
    void OnDestroy();
    void SetAssetRelativePath(stdfs::path path);
    void SetAssetAbsolutePath(stdfs::path path);
    void SetAssetCacheRelativePath(stdfs::path path);
    void SetAssetCacheAbsolutePath(stdfs::path path);
    bool SerializeToFile();

    auto GetAssetRelativePath() const -> const stdfs::path & {
        return _assetRelativePath;
    }
    auto GetAssetAbsolutePath() const -> const stdfs::path & {
        return _assetAbsolutePath;
    }
    auto GetAssetCacheRelativePath() const -> const stdfs::path & {
        return _assetCacheRelativePath;
    }
    auto GetAssetCacheAbsolutePath() const -> const stdfs::path & {
        return _assetCacheAbsolutePath;
    }

    static auto ToAssetPath(const stdfs::path &relativePath) -> stdfs::path {
        Assert(relativePath.is_relative());
	    return GetInstance()->GetAssetAbsolutePath() / relativePath;
    }
    static auto ToCachePath(const stdfs::path &relativePath) -> stdfs::path {
        Assert(relativePath.is_relative());
	    return GetInstance()->GetAssetCacheAbsolutePath() / relativePath;
    }
private:
    constexpr static std::string_view sSerializePath = "AssetProjectSetting.json";
    static void RepairPath(stdfs::path &relativePath, stdfs::path &absolutePath);
private:
    stdfs::path _assetRelativePath;
    stdfs::path _assetAbsolutePath;
    stdfs::path _assetCacheRelativePath;
    stdfs::path _assetCacheAbsolutePath;
};
