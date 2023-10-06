#pragma once
#include <cstdint>
#include <vector>
#include <bitset>

#include "InstaceIDMap.h"
#include "Foundation/NonCopyable.h"
#include "InstanceID.h"

class InstanceIDMap : NonCopyable {
public:
	InstanceIDMap();
	~InstanceIDMap();
	bool Insert(InstanceID instanceID);
	bool Remove(InstanceID instanceID);
	bool Contains(InstanceID instanceID) const;
	auto Alloc() -> InstanceID;
private:
	struct Level0PageTable;
	struct Level1PageTable;
	struct Level2PageTable;
	mutable std::mutex _mutex;
	std::unique_ptr<Level2PageTable> _pPageTables;
};

struct InstanceIDMap::Level0PageTable {
	constexpr static uint64_t kPageElementCount = 1024;
public:
	Level0PageTable();
	bool Insert(InstanceID instanceID);
	bool Remove(InstanceID instanceID);
	bool Contains(InstanceID instanceID) const;
	auto Alloc(size_t baseIndex) -> InstanceID;
	bool IsPageFree() const;
private:
	std::array<std::bitset<64>, 16> _pageTables;
};

struct InstanceIDMap::Level1PageTable {
	constexpr static uint64_t kPageElementCount = 1024 * Level0PageTable::kPageElementCount;
public:
	Level1PageTable();
	bool Insert(InstanceID instanceID);
	bool Remove(InstanceID instanceID);
	bool Contains(InstanceID instanceID) const;
	auto Alloc(size_t baseIndex) -> InstanceID;
	bool IsPageFree() const;
private:
	std::array<std::bitset<64>, 16> _mask;
	std::vector<std::unique_ptr<Level0PageTable>> _pageTables;
};

struct InstanceIDMap::Level2PageTable {
public:
	Level2PageTable();
	bool Insert(InstanceID instanceID);
	bool Remove(InstanceID instanceID);
	bool Contains(InstanceID instanceID) const;
	auto Alloc() -> InstanceID;
private:
	std::array<std::bitset<64>, 16> _mask;
	std::vector<std::unique_ptr<Level1PageTable>> _pageTables;
};