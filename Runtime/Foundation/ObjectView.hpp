#pragma once
#include "Exception.h"
#include "Foundation/PreprocessorDirectives.h"

class EmptyObjectView final : public Exception {};

template<typename T>
    requires(!std::is_reference_v<T>)
class ObjectView {
public:
    using ValueType = T;
    using ValuePtrType = ValueType *;
    using ValueRefType = ValueType &;
public:
    ObjectView() = default;
    ObjectView(std::nullptr_t) noexcept : _ptr(nullptr) {
    }
    ObjectView(T &object) : _ptr(&object) {
    }
    ObjectView(T &&object) : _ptr(&object) {
    }

    ObjectView(const ObjectView &) noexcept = default;

    ObjectView(ObjectView &&) noexcept = default;

    template<typename Other>
        requires requires { std::declval<ValuePtrType &>() = std::declval<Other *>(); }
    ObjectView(ObjectView<Other> other) {
        *this = other;
    }

    template<typename Other>
        requires requires { std::declval<ValuePtrType &>() = std::declval<Other *>(); }
    auto operator=(ObjectView<Other> other) -> ObjectView & {
        if (other.HasValue()) {
	        return operator=(*other);
        }
	    _ptr = nullptr;
        return *this;
    }

    auto operator=(const ObjectView &) noexcept -> ObjectView & = default;

    auto operator=(ObjectView &&) -> ObjectView & = default;

    auto operator=(ValueType &object) -> ObjectView & {
        _ptr = &object;
        return *this;
    }

    auto operator=(ValueType &&other) -> ObjectView & {
	    _ptr = &other;
        return *this;
    }

    auto Value() const -> ValueType & {
#ifdef MODE_DEBUG
        if (_ptr == nullptr) {
            EmptyObjectView::Throw("ObjectView is empty!");
        }
#endif
        return *_ptr;
    }

    auto ValueOr(const ValueType &other) -> ValueType {
        if (_ptr != nullptr) {
            return *_ptr;
        }
        return other;
    }

    void Reset() noexcept {
	    _ptr = nullptr;
    }

    bool HasValue() const noexcept {
        return _ptr != nullptr;
    }
    Inline(2) auto operator->() const noexcept -> ValueType * {
        return _ptr;
    }
    Inline(2) auto operator*() const noexcept -> ValueType & {
        return *_ptr;
    }
    Inline(2) explicit operator bool() const noexcept {
        return _ptr != nullptr;
    }
    friend auto operator<=>(const ObjectView &, const ObjectView &) noexcept = default;

    friend void swap(ObjectView &lhs, ObjectView &rhs) noexcept {
        std::swap(lhs._ptr, rhs._ptr);
    }

    friend std::hash<ObjectView>;
private:
    ValuePtrType _ptr = nullptr;
};

template<typename T>
ObjectView<T> MakeObjectView(T &&object) {
	return { object };
}

namespace std {

template<typename T>
struct hash<ObjectView<T>> {
    using argument_type = ObjectView<T>;
    using result_type = std::size_t;

    [[nodiscard]]
    result_type
    operator()(const argument_type &objectView) const {
        std::hash<void *> hasher;
        return hasher(objectView._ptr);
    }
};

}    // namespace std
