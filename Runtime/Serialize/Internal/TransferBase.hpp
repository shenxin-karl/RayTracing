#pragma once
#include <string_view>
#include <unordered_map>
#include "Foundation/NonCopyable.h"
#include "Foundation/TypeAlias.h"
#include "Foundation/NamespeceAlias.h"

template<typename T>
struct TransferHelper;

struct ITransferWriter {
    consteval static bool IsReading() {
        return false;
    }
};

struct ITransferReader {
    constexpr static bool IsReading() {
        return true;
    }
};

class TransferBase : NonCopyable {
public:
    TransferBase(const stdfs::path &path) : _sourceFilePath(path) {
    }
    virtual ~TransferBase() {
    }

    virtual void TransferVersion(std::string_view name, int version) {
        _currentVersion = version;
    }

    virtual void Transfer(std::string_view name, bool &data) = 0;

    virtual void Transfer(std::string_view name, uint8 &data) = 0;
    virtual void Transfer(std::string_view name, uint16 &data) = 0;
    virtual void Transfer(std::string_view name, uint32 &data) = 0;
    virtual void Transfer(std::string_view name, uint64 &data) = 0;

    virtual void Transfer(std::string_view name, int8 &data) = 0;
    virtual void Transfer(std::string_view name, int16 &data) = 0;
    virtual void Transfer(std::string_view name, int32 &data) = 0;
    virtual void Transfer(std::string_view name, int64 &data) = 0;

    virtual void Transfer(std::string_view name, float &data) = 0;
    virtual void Transfer(std::string_view name, double &data) = 0;

    virtual void Transfer(std::string_view name, std::string &data) = 0;

    virtual bool BeginTransfer() = 0;
    virtual bool EndTransfer() {
        _currentVersion = 0;
        _versionMap.clear();
        return true;
    }

    void SetVersion(std::string_view name, int version) {
        _versionMap[std::string(name)] = version;
    }
    auto GetVersion(std::string_view name) -> int {
        auto iter = _versionMap.find(name.data());
        if (iter != _versionMap.end()) {
            return iter->second;
        }
        return 0;
    }
protected:
    size_t _currentVersion = 0;
    stdfs::path _sourceFilePath;
    std::unordered_map<std::string, int> _versionMap;
};

template<typename T>
concept TransferContextConcept = std::is_base_of_v<TransferBase, T>;

class TransferJsonReader;
class TransferJsonWriter;

#define DECLARE_SERIALIZER(Type)                                                                                       \
private:                                                                                                               \
    template<TransferContextConcept T>                                                                                 \
    void TransferImpl(T &transfer);                                                                                    \
public:                                                                                                                \
    template<TransferContextConcept T>                                                                                 \
    void Transfer(T &transfer) {                                                                                       \
        TransferImpl(transfer);                                                                                        \
    }

#define IMPLEMENT_SERIALIZER(Type)                                                                                     \
    template void Type::TransferImpl<TransferJsonWriter>(TransferJsonWriter & transfer);                               \
    template void Type::TransferImpl<TransferJsonReader>(TransferJsonReader & transfer);

#define DECLARE_VIRTUAL_SERIALIZER(Type)                                                                               \
public:                                                                                                                \
    template<TransferContextConcept T>                                                                                 \
    void TransferImpl(T &transfer);                                                                                    \
    virtual void Transfer(TransferJsonWriter &transfer);                                                               \
    virtual void Transfer(TransferJsonReader &transfer);

#define IMPLEMENT_VIRTUAL_SERIALIZER(Type)                                                                             \
    void Type::Transfer(TransferJsonWriter &transfer) {                                                                \
        Type::TransferImpl(transfer);                                                                                  \
    }                                                                                                                  \
    void Type::Transfer(TransferJsonReader &transfer) {                                                                \
        Type::TransferImpl(transfer);                                                                                  \
    }

#define TRANSFER(field) TransferHelper<decltype(this->field)>::Transfer(transfer, #field, field)
#define TRANSFER_WITH_NAME(field, name) TransferHelper<decltype(this->field)>::Transfer(transfer, name, field)
#define TRANSFER_VERSION(TypeKey, Version) transfer.TransferVersion(TypeKey, Version)

template<TransferContextConcept Transfer, typename T>
bool Serialize(Transfer &transfer, T &object) {
    if (!transfer.BeginTransfer()) {
        return false;
    }
    object.Transfer(transfer);
    return transfer.EndTransfer();
}