#pragma once
#include "NonCopyable.h"
#include "MainThread.h"
#include "Exception.h"
#include "PreprocessorDirectives.h"

template<typename T>
class Singleton : private NonCopyable {
public:
 	Inline(2) static auto OnInstanceCreate() -> T * {
		MainThread::EnsureMainThread();
		Exception::CondThrow(_pObject == nullptr, "_pObject == nullptr!");
		_pObject = new T();
		return _pObject;
	}
	Inline(2) static void OnInstanceDestroy() {
		MainThread::EnsureMainThread();
		Exception::CondThrow(_pObject != nullptr, "_pObject != nullptr!");
		T *ptr = _pObject;
		_pObject = nullptr;
		delete ptr;
	}
	Inline(2) static auto GetInstance() -> T * {
 		return _pObject;
 	}
	Inline(2) virtual ~Singleton() {
		if (_pObject != nullptr) {
			Exception::Throw("Singleton object not destroy!");
		}
 	}
private:
	static inline T *_pObject = nullptr;
};
