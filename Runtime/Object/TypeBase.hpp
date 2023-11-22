#pragma once
#include "Foundation/TypeTraits.hpp"
#include "Foundation/NonCopyable.h"
#include <string_view>
#include <typeindex>

#define DECLARE_TYPEID(type) kTypeId##type
enum TypeId {
    DECLARE_TYPEID(TypeBase) = 0,
    DECLARE_TYPEID(AssetProjectSetting) = 1,
    DECLARE_TYPEID(Object) = 50,

    DECLARE_TYPEID(Component) = 200,
};
#undef DECLARE_TYPEID

class TypeBase : private NonCopyable {
public:
    using SuperType = void;
    using ThisType = TypeBase;
    virtual ~TypeBase() {
    }
    virtual auto GetTypeId() const -> TypeId {
        return kTypeIdTypeBase;
    }
    virtual auto GetTypeName() const -> std::string_view {
        return "TypeBase";
    }
    virtual auto GetTypeIndex() const -> std::type_index {
        return typeid(TypeBase);
    }
};

#define DECLARE_CLASS(Type)                                                                                            \
public:                                                                                                                \
    using SuperType = ThisType;                                                                                        \
    using ThisType = Type;                                                                                             \
    auto GetTypeId() const->TypeId override {                                                                          \
        return kTypeId##Type;                                                                                          \
    }                                                                                                                  \
    auto GetTypeName() const->std::string_view override {                                                              \
        return #Type;                                                                                                  \
    }                                                                                                                  \
    auto GetTypeIndex() const->std::type_index override {                                                              \
        return typeid(Type);                                                                                           \
    }

template<typename T>
auto GetTypeIndex() -> std::type_index {
	return std::type_index(typeid(T));
}

template<typename T> requires(std::is_base_of_v<TypeBase, T>)
auto GetTypeIndex(const TypeBase *pObject) -> std::type_index {
	return pObject->GetTypeIndex();
} 