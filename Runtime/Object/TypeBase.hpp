#pragma once
#include "Foundation/TypeTraits.hpp"
#include "Foundation/NonCopyable.h"
#include <string_view>

#define DECLARE_TYPEID(type) kTypeId##type
enum TypeId {
    DECLARE_TYPEID(TypeBase) = 0,
    DECLARE_TYPEID(AssetProjectSetting) = 1,
    DECLARE_TYPEID(Object) = 50,
};
#undef DECLARE_TYPEID

class TypeBase : private NonCopyable {
public:
    using SuperType = void;
    using ThisType = TypeBase;
    virtual ~TypeBase() {
    }
    virtual TypeId GetTypeId() const {
        return kTypeIdTypeBase;
    }
    virtual std::string_view GetTypeName() const {
        return "TypeBase";
    }
};

#define DECLARE_CLASS(Type)                                                                                            \
public:                                                                                                                \
    using SuperType = ThisType;                                                                                        \
    using ThisType = Type;                                                                                             \
    TypeId GetTypeId() const override {                                                                                \
        return kTypeId##Type;                                                                                          \
    }                                                                                                                  \
    std::string_view GetTypeName() const override {                                                                   \
        return #Type;                                                                                                  \
    }
