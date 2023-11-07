#include "MemoryStream.h"
#include "Exception.h"
#include <algorithm>

MemoryStream::MemoryStream() : _pData(nullptr), _size(0), _capacity(0) {
}

MemoryStream::~MemoryStream() {
    if (_pData != nullptr) {
        free(_pData);
        _pData = nullptr;
        _size = 0;
        _capacity = 0;
    }
}

MemoryStream::MemoryStream(MemoryStream &&other) noexcept
    : _pData(std::exchange(other._pData, nullptr)),
      _size(std::exchange(other._size, 0)),
      _capacity(std::exchange(other._capacity, 0)) {
}

MemoryStream &MemoryStream::operator=(MemoryStream &&other) noexcept {
    _pData = std::exchange(other._pData, nullptr);
    _size = std::exchange(other._size, 0);
    _capacity = std::exchange(other._capacity, 0);
    return *this;
}

void MemoryStream::Append(const void *pData, size_t dataSize) {
    if (_size + dataSize > _capacity) {
        Reverse(std::max<size_t>(32, _capacity * 2));
    }
    std::memcpy(_pData + _size, pData, dataSize);
    _size += dataSize;
}

void MemoryStream::Reverse(size_t capacity) {
    Assert(capacity > 0);
    if (_capacity < capacity) {
        _capacity = std::max<size_t>(32, capacity);
        _pData = static_cast<std::byte *>(realloc(_pData, _capacity));
    }
}

void MemoryStream::Resize(size_t size, std::byte value) {
    Assert(size > 0);
    Reverse(size);
    _size = size;
    std::memset(_pData, static_cast<int>(value), size);
}