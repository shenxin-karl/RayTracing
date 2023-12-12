#pragma once
#include "Foundation/Singleton.hpp"
#include <list>

class RefCounter;
class GarbageCollection : public Singleton<GarbageCollection> {
public:
	GarbageCollection();
	~GarbageCollection() override;
public:
	void OnCreate();
	void OnDestroy();
	void SetDelayedReleaseFrames(size_t num);
	void AddObject(RefCounter *pObject);
	void DoGCWork();
	void OnPostRender(GameTimer &timer);
private:
	void Lock();
	void UnLock();
	struct Node {
		uint64_t	releaseFrameIndex;
		RefCounter *pObject = nullptr;
	};
private:
	uint64_t			_frameIndex;
	size_t				_delayedReleaseFrames;
	std::list<Node>		_objects;
	std::atomic_flag	_lock;
};
