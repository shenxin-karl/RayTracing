#include "GarbageCollection.h"
#include "RefCounter.h"

GarbageCollection::GarbageCollection() : _frameIndex(0), _delayedReleaseFrames(5) {
}

GarbageCollection::~GarbageCollection() {
}

void GarbageCollection::OnCreate() {
}

void GarbageCollection::OnDestroy() {
    _frameIndex = std::numeric_limits<uint64_t>::max();
    DoGCWork();
}

void GarbageCollection::SetDelayedReleaseFrames(size_t num) {
    Assert(num >= 1);
    _delayedReleaseFrames = num;
}

void GarbageCollection::AddObject(RefCounter *pObject) {
    Assert(pObject->GetRefCount() == 0);
    Node node = {
        _frameIndex,
        pObject,
    };

    Lock();
    _objects.push_back(node);
    UnLock();
}

void GarbageCollection::DoGCWork() {
    MainThread::EnsureMainThread();
    if (_frameIndex < _delayedReleaseFrames) {
        return;
    }

    uint64_t freeReleaseFrameIndex = _frameIndex - _delayedReleaseFrames;
    Lock();
    auto iter = _objects.begin();
    auto end = _objects.end();
    UnLock();

    while (iter != end) {
        if (freeReleaseFrameIndex >= iter->releaseFrameIndex) {
            delete iter->pObject;
            iter = _objects.erase(iter);
        } else {
            return;
        }
    }
}

void GarbageCollection::OnPostRender(GameTimer &timer) {
    DoGCWork();
    ++_frameIndex;
}

void GarbageCollection::Lock() {
     while (_lock.test_and_set(std::memory_order_acquire)) {
	     std::this_thread::yield();
     }
}

void GarbageCollection::UnLock() {
    _lock.clear(std::memory_order_release);               
}
