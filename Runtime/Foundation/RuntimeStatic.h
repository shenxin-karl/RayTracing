#pragma once
#include <cassert>
#include <cstddef>
#include <type_traits>
#include "PreprocessorDirectives.h"
#include "Foundation/NonCopyable.h"

template<typename T>
class RuntimeStatic : NonCopyable {
public:
    RuntimeStatic() {
        static_assert(std::is_constructible_v<T>, "This Type not member func 'Initialize' or 'Destroy'");
        _pObject = new T;
    }
    ~RuntimeStatic() {
        delete _pObject;
        _pObject = nullptr;
    }
    Inline(2) T *Get() {
        return _pObject;
    }
    Inline(2) T *operator->() {
        return _pObject;
    }
    Inline(2) operator T*() const {
        return _pObject;
    }
    Inline(2) explicit operator bool() const {
        return _pObject != nullptr;
    }
    Inline(2) friend bool operator!=(const RuntimeStatic &lhs, std::nullptr_t) {
        return lhs._pObject != nullptr;
    }
    Inline(2) friend bool operator!=(std::nullptr_t, const RuntimeStatic &rhs) {
        return rhs._pObject != nullptr;
    }
    Inline(2) friend bool operator==(const RuntimeStatic &lhs, std::nullptr_t) {
        return lhs._pObject == nullptr;
    }
    Inline(2) friend bool operator==(std::nullptr_t, const RuntimeStatic &rhs) {
        return rhs._pObject == nullptr;
    }
private:
    T *_pObject = nullptr;
};