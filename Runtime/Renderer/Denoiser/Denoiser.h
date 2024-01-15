#pragma once
#include <memory>
#include "NRI.h"
#include "NRD.h"
#include "NRDSettings.h"
#include "Foundation/NonCopyable.h"

class NriInterface;
class NrdIntegration;

class Denoiser : private NonCopyable {
public:
    void OnCreate();
    void OnDestroy();
    void NewFrame();
    void SetCommonSetting(const nrd::CommonSettings &settings);
    void OnResize(size_t width, size_t height);
private:
    void CreateNRD(size_t width, size_t height);
private:
    std::unique_ptr<NrdIntegration> _pNrd;
    nri::Device *_pNriDevice = nullptr;
    std::unique_ptr<NriInterface> _pNriInterface;
};