#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include <functional>
#include "Foundation/ITick.hpp"

class InputSystem;
class GameTimer;

class Window : public ITick {
public:
    class WindowClass;
    Window(int width, int height, const std::string &title, InputSystem *pInputSystem);
    ~Window() override;
    bool ShouldClose() const;
    void SetShouldClose(bool flag);
    auto GetReturnCode() const -> int;
    void SetMessageDispatchCallback(const std::function<void(HWND, UINT, WPARAM, LPARAM)> &callback);
    void SetPrepareMessageCallBack(const std::function<bool(HWND, UINT, WPARAM, LPARAM)> &callback);
    void SetResizeCallback(const std::function<void(int, int)> &callback);
    auto GetWidth() const -> int;
    auto GetHeight() const -> int;
    auto GetHWND() const -> HWND;
    auto GetAspectRatio() const -> float;
    auto GetTitle() const -> const std::string &;
    void SetShowTitle(const std::string &title);
    bool IsPaused() const;
    void PollEvent(GameTimer &timer);
    void OnPreUpdate(GameTimer &timer) override;
private:
    static void CenterWindow(HWND hwnd);
    static LRESULT CALLBACK HandleMsgSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HandleMsgThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Stop();
    void Start();
private:
    HWND _hwnd;
    int _width;
    int _height;
    bool _shouldClose;
    int _result;
    std::string _title;
    InputSystem *_pInputSystem = nullptr;
    std::function<void(HWND, UINT, WPARAM, LPARAM)> _messageDispatchCallback;
    std::function<bool(HWND, UINT, WPARAM, LPARAM)> _prepareMessageCallBack;
    std::function<void(int x, int y)> _resizeCallback;
    GameTimer *_pGameTimer = nullptr;
public:
    bool _paused = false;
    bool _minimized = false;
    bool _maximized = false;
    bool _fullScreenState = false;
    bool _resizeDirty = false;
};

class Window::WindowClass {
    HINSTANCE hInstance_ = nullptr;
private:
    using HandleMessageFuncType = LRESULT(CALLBACK *)(HWND, UINT, WPARAM, LPARAM);
    static std::unique_ptr<WindowClass> _pSingleton;
public:
    WindowClass();
    ~WindowClass();
    static HINSTANCE GetInstance();
    static const wchar_t *GetClassName();
    static inline HandleMessageFuncType pHandleMessageFunc = &(DefWindowProc);
};
