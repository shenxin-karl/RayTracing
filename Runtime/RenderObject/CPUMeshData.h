#pragma once
#include <memory>
#include "VertexSemantic.hpp"
#include "Foundation/Exception.h"
#include "Foundation/NonCopyable.h"

class CPUMeshData : private NonCopyable {
public:
    class StrideIterator;
    CPUMeshData();
    ~CPUMeshData();
public:
    auto GetSemanticBegin(SemanticIndex index) -> StrideIterator;
    auto GetSemanticEnd(SemanticIndex index) -> StrideIterator;
    void Resize(SemanticMask mask, size_t vertexCount, size_t indexCount);
public:
    auto GetVertices() const -> const int8_t * {
        return _pVertices.get();
    }
    auto GetIndices() const -> const int16_t * {
        return _pIndices.get();
    }
    auto GetVertexCount() const -> size_t {
        return _vertexCount;
    }
    auto GetIndexCount() const -> size_t {
        return _indexCount;
    }
    auto GetIndicesBegin() -> int16_t * {
        return _pIndices.get();
    }
    auto GetIndicesEnd() -> int16_t * {
        return _pIndices.get() + _indexCount;
    }
    auto GetSemanticMask() const -> SemanticMask {
        return _semanticMask;
    }
private:
    // clang-format off
	SemanticMask				_semanticMask;
	size_t						_vertexCount;
	size_t						_indexCount;
	std::unique_ptr<int8_t[]>	_pVertices;
	std::unique_ptr<int16_t[]>	_pIndices;
    // clang-format on
};

class CPUMeshData::StrideIterator {
public:
    StrideIterator(int8_t *ptr, size_t stride, size_t dataSize) : _ptr(ptr), _stride(stride), _dataSize(dataSize) {
    }
    StrideIterator(const StrideIterator &) = default;
    StrideIterator &operator=(const StrideIterator &) = default;

    friend bool operator==(const StrideIterator &lhs, const StrideIterator &rhs) {
        return lhs._ptr == rhs._ptr && lhs._stride == rhs._stride && lhs._dataSize == rhs._dataSize;
    }
    friend bool operator!=(const StrideIterator &lhs, const StrideIterator &rhs) {
        return !(lhs == rhs);
    }
    auto operator++() -> StrideIterator & {
        _ptr += _stride;
        return *this;
    }
    auto operator++(int) -> StrideIterator {
        ++(*this);
        return *this;
    }
    auto operator--() -> StrideIterator & {
        _ptr -= _stride;
        return *this;
    }
    auto operator--(int) -> StrideIterator {
        --(*this);
        return *this;
    }
    auto operator+(size_t index) const -> StrideIterator {
        StrideIterator ret = *this;
        ret._ptr += (index * ret._dataSize);
        return ret;
    }
    auto operator+=(size_t index) -> StrideIterator & {
		*this = (*this) + index;
        return *this;
    }
    auto operator-(size_t index) const -> StrideIterator {
        StrideIterator ret = *this;
        ret._ptr -= (index * ret._dataSize);
        return ret;
    }
    auto operator-=(size_t index) -> StrideIterator & {
		*this = (*this) - index;
        return *this;
    }
    template<typename T>
    auto Set(const T &data) -> T & {
        Get<T>() = data;
        return Get<T>();
    }

    template<typename T>
    auto Get() -> T & {
        Assert(sizeof(T) == _dataSize);
        return *reinterpret_cast<T *>(_ptr);
    }
private:
    // clang-format off
	int8_t	*_ptr;
	size_t	 _stride;
	size_t	 _dataSize;
    // clang-format on
};