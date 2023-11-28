#pragma once
#include <unordered_map>
#include <functional>
#include "Foundation/NonCopyable.h"
#include "Foundation/MainThread.h"

struct ICallbackList;
struct CallbackHandle : private NonCopyable {
public:
    CallbackHandle() = default;
    CallbackHandle(CallbackHandle &&other) noexcept;
    CallbackHandle &operator=(CallbackHandle &&other) noexcept;

    ~CallbackHandle();
    void Release();
private:
    template<typename... Args>
    friend class CallbackList;
    int _id = -1;
    ICallbackList *_pCallbackList = nullptr;
};

struct ICallbackList : private NonCopyable {
	virtual void Remove(const CallbackHandle &) = 0;
    virtual ~ICallbackList() = default;
};

template<typename... Args>
class CallbackList : public ICallbackList {
public:
    CallbackList() : _handle(0) {
    }

    template<typename T>
    auto Register(T *pObject, void (T::*pMemFunc)(Args...)) -> CallbackHandle {
        return Register([=](Args... args) -> void { (pObject->*pMemFunc)(std::forward<Args>(args)...); });
    }

    auto Register(std::function<void(Args...)> function) -> CallbackHandle {
        MainThread::EnsureMainThread();
        CallbackHandle handle;
        handle._id = _handle++;
        handle._pCallbackList = this;
        _callbacks[handle._id] = std::move(function);
        return handle;
    }

    void Invoke(Args&&... args) const {
        MainThread::EnsureMainThread();
        for (auto &&[_, callback] : _callbacks) {
            callback(args...);
        }
    }

    void Remove(const CallbackHandle &handle) override {
        MainThread::EnsureMainThread();
	    _callbacks.erase(handle._id);
    }
private:
    // clang-format off
    using CallbackContainer = std::unordered_map<int, std::function<void(Args...)>>;
    int                _handle;
     CallbackContainer _callbacks;
    // clang-format on
};


struct GlobalCallbacks {
	CallbackList<GameTimer &>   onPreUpdate;
    CallbackList<GameTimer &>   onUpdate;
    CallbackList<GameTimer &>   onPostUpdate;

    CallbackList<GameTimer &>   onPreRender;
    CallbackList<GameTimer &>   onRender;
    CallbackList<GameTimer &>   onPostRender;

    // void(size_t width, size_t height)
    CallbackList<size_t, size_t> onResize;

    static GlobalCallbacks &Get() {
	    static GlobalCallbacks instance;
        return instance;
    }
};