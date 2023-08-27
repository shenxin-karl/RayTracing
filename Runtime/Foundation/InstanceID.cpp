#include "InstanceID.h"
#include "Serialize/Internal/TransferHelper.hpp"

IMPLEMENT_SERIALIZER(InstanceID);

template<TransferContextConcept T>
void InstanceID::TransferImpl(T &transfer) {
	TRANSFER(_id);
}
