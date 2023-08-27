#include "InstaceIDMap.h"

#include "Exception.h"

static constexpr uint64 kPageCapacity = 1024;
static constexpr uint64 kMaskBitSize = 64;

InstanceIDMap::InstanceIDMap() {
	_pPageTables = std::make_unique<Level2PageTable>();
}

InstanceIDMap::~InstanceIDMap() {

}

bool InstanceIDMap::Insert(InstanceID instanceID) {
	std::unique_lock lock(_mutex);
	return _pPageTables->Insert(instanceID);
}

bool InstanceIDMap::Remove(InstanceID instanceID) {
	std::unique_lock lock(_mutex);
	return _pPageTables->Remove(instanceID);
}

bool InstanceIDMap::Contains(InstanceID instanceID) const {
	std::unique_lock lock(_mutex);
	return _pPageTables->Contains(instanceID);
}

auto InstanceIDMap::Alloc() -> InstanceID {
	std::unique_lock lock(_mutex);
	return _pPageTables->Alloc();
}

InstanceIDMap::Level0PageTable::Level0PageTable() {
}

bool InstanceIDMap::Level0PageTable::Insert(InstanceID instanceID) {
	size_t bitIndex = instanceID.GetID() % kPageElementCount;
	size_t elementIndex = bitIndex / kMaskBitSize;
	if (_pageTables[elementIndex].test(bitIndex)) {
		return false;
	}
	_pageTables[elementIndex].set(bitIndex, true);
	return true;
}

bool InstanceIDMap::Level0PageTable::Remove(InstanceID instanceID) {
	size_t bitIndex = instanceID.GetID() % kPageElementCount;
	size_t elementIndex = bitIndex / kMaskBitSize;
	if (!_pageTables[elementIndex].test(bitIndex)) {
		return false;
	}
	_pageTables[elementIndex].set(bitIndex, false);
	return true;
}

bool InstanceIDMap::Level0PageTable::Contains(InstanceID instanceID) const {
	size_t bitIndex = instanceID.GetID() % kMaskBitSize;
	size_t elementIndex = instanceID.GetID() / kMaskBitSize;
	return _pageTables[elementIndex].test(bitIndex);
}

auto InstanceIDMap::Level0PageTable::Alloc(size_t baseIndex) -> InstanceID {
	for (size_t i = 0; i < 16; ++i) {
		if (_pageTables[i].all()) {
			continue;
		}
		for (size_t j = 0; j < 64; ++j) {
			if (!_pageTables[i].test(j)) {
				_pageTables[i].set(j, true);
				size_t id = baseIndex + i * 64 + j;
				return InstanceID{id};
			}
		}
	}
	Exception::Throw("Alloc Failed!");
	return InstanceID::Invalid();
}

bool InstanceIDMap::Level0PageTable::IsPageFree() const {
	for (size_t i = 0; i < 16; ++i) {
		if (!_pageTables[i].all()) {
			return true;
		}
	}
	return false;
}

InstanceIDMap::Level1PageTable::Level1PageTable() {
	_pageTables.resize(kPageCapacity);
	for (size_t i = 0; i < 16; ++i) {
		_mask[i].flip();
	}
}

bool InstanceIDMap::Level1PageTable::Insert(InstanceID instanceID) {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = (id / Level0PageTable::kPageElementCount) % kPageCapacity;
	if (_pageTables[pageIndex] == nullptr) {
		_pageTables[pageIndex] = std::make_unique<Level0PageTable>();
	}
	if (_pageTables[pageIndex]->Insert(instanceID)) {
		uint64 maskIndex = pageIndex / kPageCapacity;
		uint64 bitIndex = pageIndex % kPageCapacity;
		_mask[maskIndex].set(bitIndex, _pageTables[pageIndex]->IsPageFree());
		return true;
	}
	return false;
}

bool InstanceIDMap::Level1PageTable::Remove(InstanceID instanceID) {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = (id / Level0PageTable::kPageElementCount) % kPageCapacity;
	if (_pageTables[pageIndex] == nullptr) {
		_pageTables[pageIndex] = std::make_unique<Level0PageTable>();
	}
	if (_pageTables[pageIndex]->Remove(instanceID)) {
		uint64 maskIndex = pageIndex / kPageCapacity;
		uint64 bitIndex = pageIndex % kPageCapacity;
		_mask[maskIndex].set(bitIndex, false);
		return true;
	}
	return false;
}

bool InstanceIDMap::Level1PageTable::Contains(InstanceID instanceID) const {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = (id / Level0PageTable::kPageElementCount) % kPageCapacity;
	if (_pageTables[pageIndex] == nullptr) {
		return false;
	}
	return _pageTables[pageIndex]->Contains(instanceID);
}

auto InstanceIDMap::Level1PageTable::Alloc(size_t baseIndex) -> InstanceID {
	for (size_t i = 0; i < 16; ++i) {
		if (_mask[i].none()) {
			continue;
		}
		if (_pageTables[i] == nullptr) {
			_pageTables[i] = std::make_unique<Level0PageTable>();
		}

		InstanceID instanceId = _pageTables[i]->Alloc(baseIndex + i * Level0PageTable::kPageElementCount);
		_mask[i].set(i, _pageTables[i]->IsPageFree());
		return instanceId;
	}
	return InstanceID::Invalid();
}

bool InstanceIDMap::Level1PageTable::IsPageFree() const {
	for (size_t i = 0; i < kMaskBitSize; ++i) {
		if (!_mask[i].all()) {
			return true;
		}
	}
	return false;
}

InstanceIDMap::Level2PageTable::Level2PageTable() {
	_pageTables.resize(kPageCapacity);
	for (size_t i = 0; i < 16; ++i) {
		_mask[i].flip();
	}
}

bool InstanceIDMap::Level2PageTable::Insert(InstanceID instanceID) {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = id / Level1PageTable::kPageElementCount;
	if (_pageTables[pageIndex] == nullptr) {
		_pageTables[pageIndex] = std::make_unique<Level1PageTable>();
	}
	if (_pageTables[pageIndex]->Insert(instanceID)) {
		uint64 maskIndex = pageIndex / kMaskBitSize;
		uint64 bitIndex = pageIndex % kMaskBitSize;
		_mask[maskIndex].set(bitIndex, _pageTables[pageIndex]->IsPageFree());
		return true;
	}
	return false;
}

bool InstanceIDMap::Level2PageTable::Remove(InstanceID instanceID) {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = id / Level1PageTable::kPageElementCount;
	if (_pageTables[pageIndex] == nullptr) {
		return false;
	}
	if (_pageTables[pageIndex]->Remove(instanceID)) {
		uint64 maskIndex = pageIndex / kMaskBitSize;
		uint64 bitIndex = pageIndex % kMaskBitSize;
		_mask[maskIndex].set(bitIndex, true);
		return true;
	}
	return false;
}

bool InstanceIDMap::Level2PageTable::Contains(InstanceID instanceID) const {
	uint64 id = instanceID.GetID();
	uint64 pageIndex = id / Level1PageTable::kPageElementCount;
	if (_pageTables[pageIndex] == nullptr) {
		return false;
	}
	return _pageTables[pageIndex]->Contains(instanceID);
}

auto InstanceIDMap::Level2PageTable::Alloc() -> InstanceID {
	for (size_t maskIndex = 0; maskIndex < 16; ++maskIndex) {
		if (_mask[maskIndex].none()) {
			continue;
		}

		size_t basePageIndex = maskIndex * 64;
		for (size_t bitIndex = 0; bitIndex < 64; ++bitIndex) {
			if (!_mask[maskIndex].test(bitIndex)) {
				continue;
			}
			size_t pageIndex = basePageIndex + bitIndex;
			if (_pageTables[pageIndex] == nullptr) {
				_pageTables[pageIndex] = std::make_unique<Level1PageTable>();
			}
			InstanceID instanceId = _pageTables[pageIndex]->Alloc(pageIndex * Level1PageTable::kPageElementCount);
			_mask[pageIndex].set(bitIndex, _pageTables[pageIndex]->IsPageFree());
			return instanceId;
		}
	}
	return InstanceID::Invalid();
}
