#include "RefCounter.h"
#include "Foundation/Exception.h"
#include "GarbageCollection.h"

void RefCounter::Release() {
	--_refCount;
	Assert(_refCount >= 0);
	if (_refCount == 0) {
		GarbageCollection::GetInstance()->AddObject(this);
	}
}
