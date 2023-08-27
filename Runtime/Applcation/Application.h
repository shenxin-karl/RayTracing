#pragma once
#include "Foundation/ITick.hpp"
#include "Foundation/Singleton.hpp"

class Renderer;
class Application
	: public ITick
	, public Singleton<Application> {
public:
    Application();
    ~Application() override;
public:
    void OnCreate();
    void OnDestroy();
    bool IsRunning() const;
    void OnPreUpdate(GameTimer &timer) override;
    void OnUpdate(GameTimer &timer) override;
    void OnPostUpdate(GameTimer &timer) override;
    void OnPreRender(GameTimer &timer) override;
    void OnRender(GameTimer &timer) override;
    void OnPostRender(GameTimer &timer) override;
    void OnResize(uint32_t width, uint32_t height);
private:
    std::unique_ptr<Renderer> _pRenderer;
};
