#include "GlobalCallbacks.h"

CallbackHandle::CallbackHandle(CallbackHandle &&other) noexcept {
	_id = std::exchange(other._id, -1);
	_pCallbackList = std::exchange(other._pCallbackList, nullptr);
}

CallbackHandle & CallbackHandle::operator=(CallbackHandle &&other) noexcept {
	_id = std::exchange(other._id, -1);
	_pCallbackList = std::exchange(other._pCallbackList, nullptr);
	return *this;
}

CallbackHandle::~CallbackHandle() {
	Release();
}

void CallbackHandle::Release() {
	if (_id != -1) {
		_pCallbackList->Remove(*this);
		_id = -1;
	}
}
