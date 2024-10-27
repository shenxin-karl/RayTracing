#include "Object.h"

IMPLEMENT_VIRTUAL_SERIALIZER(Object)

static std::atomic_int64_t sGlobalInstanceID = 0;

Object::Object() {

}

void Object::InitInstanceId() {
	_instanceId.Set(sGlobalInstanceID++);
};

template<TransferContextConcept Archive>
void Object::TransferImpl(Archive &transfer) {
	TRANSFER_VERSION("Object", 1);
	TRANSFER(_name);
	TRANSFER(_instanceId);
}
