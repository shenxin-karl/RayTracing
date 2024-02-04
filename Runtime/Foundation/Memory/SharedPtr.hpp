#pragma once
#include <type_traits>
#include "RefCounter.h"

template<typename T>
class SharedPtr {
public:
    SharedPtr(nullptr_t) noexcept : _ptr(nullptr) {
    }
    SharedPtr() noexcept : _ptr(nullptr) {
    }
    ~SharedPtr() {
        Release();
    }
    SharedPtr(T *ptr) : _ptr(ptr) {
        AddRef();
    }
    SharedPtr(const SharedPtr &other) : _ptr(other._ptr) {
        if (this == &other) {
            return;
        }
        AddRef();
    }
    template<typename U>
        requires(std::is_base_of_v<T, U>)
    SharedPtr(const SharedPtr<U> &other) : _ptr(other._ptr) {
        if (this == &other) {
            return;
        }
        AddRef();
    }
    SharedPtr(SharedPtr &&other) noexcept : _ptr(other._ptr) {
        other._ptr = nullptr;
    }
    template<typename U>
        requires(std::is_base_of_v<T, U>)
    SharedPtr(SharedPtr<U> &&other) noexcept : _ptr(other._ptr) {
        other._ptr = nullptr;
    }
    SharedPtr &operator=(const SharedPtr &other) {
        Assert(this != &other);
        Release();
        _ptr = other._ptr;
        AddRef();
        return *this;
    }
    template<typename U>
        requires(std::is_base_of_v<T, U>)
    SharedPtr &operator=(const SharedPtr<U> &other) {
        Assert(this != &other);
        Release();
        _ptr = other._ptr;
        AddRef();
        return *this;
    }
    SharedPtr &operator=(SharedPtr &&other) noexcept {
        Release();
        _ptr = other._ptr;
        other._ptr = nullptr;
        return *this;
    }
    SharedPtr &operator=(std::nullptr_t) {
        Release();
        return *this;
    }
    template<typename U>
        requires(std::is_base_of_v<T, U>)
    SharedPtr &operator=(SharedPtr<U> &&other) {
        Release();
        _ptr = other._ptr;
        other._ptr = nullptr;
        return *this;
    }
    void Release() {
        if (_ptr != nullptr) {
            static_cast<RefCounter *>(_ptr)->Release();
        }
        _ptr = nullptr;
    }
    auto Get() const -> T * {
        return _ptr;
    }
    explicit operator bool() const {
        return _ptr != nullptr;
    }
    auto operator->() const -> T * {
        return _ptr;
    }
    friend void swap(SharedPtr &lhs, SharedPtr &rhs) noexcept {
        std::swap(lhs._ptr, rhs._ptr);
    }
    friend auto operator<=>(const SharedPtr &, const SharedPtr &) = default;
    template<typename U>
    friend bool operator==(const SharedPtr &lhs, const SharedPtr<U> &rhs) {
        return lhs.Get() == rhs.Get();
    }
    template<typename U>
    friend bool operator!=(const SharedPtr &lhs, const SharedPtr<U> &rhs) {
        return !(lhs == rhs);
    }
    friend bool operator==(const SharedPtr &lhs, std::nullptr_t) {
        return lhs._ptr == nullptr;
    }
    friend bool operator==(std::nullptr_t, const SharedPtr &rhs) {
        return rhs._ptr == nullptr;
    }
    friend bool operator!=(const SharedPtr &lhs, std::nullptr_t) {
        return !(lhs == nullptr);
    }
    friend bool operator!=(std::nullptr_t, const SharedPtr &rhs) {
        return !(rhs == nullptr);
    }
private:
    void AddRef() {
        if (_ptr != nullptr) {
            static_cast<RefCounter *>(_ptr)->AddRef();
        }
    }
    template<typename>
    friend class SharedPtr;
private:
    T *_ptr;
};

template<typename T, typename... Args>
    requires(std::is_base_of_v<RefCounter, T>)
SharedPtr<T> MakeShared(Args &&...args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}