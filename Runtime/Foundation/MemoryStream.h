#pragma once
#include <cstddef>
#include <type_traits>
#include "NonCopyable.h"

class MemoryStream : public NonCopyable {
public:
    MemoryStream();
    ~MemoryStream();

    MemoryStream(MemoryStream &&other) noexcept;
    MemoryStream &operator=(MemoryStream &&other) noexcept;

    void Append(const void *pData, size_t dataSize);
    void Reverse(size_t capacity);
    void Resize(size_t size, std::byte value = std::byte{0});

    template<typename T>
        requires(std::is_trivially_copyable_v<T>)
    void Append(T &&obj) {
        Append(&obj, sizeof(T));
    }
    auto GetData() const -> void * {
        return _pData;
    }
    auto GetSize() const -> size_t {
        return _size;
    }
    auto GetCapacity() const -> size_t {
        return _capacity;
    }
private:
    // clang-format off
    std::byte  *_pData;
    std::size_t _size;
    std::size_t _capacity;
    // clang-format on
};
