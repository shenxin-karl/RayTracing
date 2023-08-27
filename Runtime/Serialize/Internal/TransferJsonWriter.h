#pragma once
#include <stack>
#include <json/value.h>

class TransferJsonWriter;

namespace JsonWriteDetail {

template<typename T>
concept InvokeObjectTransfer = requires(T a) { a.Transfer(std::declval<TransferJsonWriter &>()); };

template<typename T>
concept IsSTLContainer = IsStdVector<T>::value || IsStdList<T>::value || IsStdSet<T>::value || IsStdMap<T>::value ||
                         IsStdMultiSet<T>::value || IsStdMultiMap<T>::value || IsStdUnorderedSet<T>::value ||
                         IsStdUnorderedMultiSet<T>::value || IsStdUnorderedMap<T>::value ||
                         IsStdUnorderedMultiMap<T>::value;

}    // namespace JsonWriteDetail

class TransferJsonWriter : public TransferBase, public ITransferWriter {
public:
    using TransferBase::TransferBase;
    void TransferVersion(std::string_view name, int version) final;
    void Transfer(std::string_view name, bool &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, uint8 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, uint16 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, uint32 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, uint64 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, int8 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, int16 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, int32 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, int64 &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, float &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, double &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    void Transfer(std::string_view name, std::string &data) final {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }
    template<JsonWriteDetail::IsSTLContainer T>
    void Transfer(std::string_view name, T &data) {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }

    template<JsonWriteDetail::InvokeObjectTransfer T>
    void Transfer(std::string_view name, T &data) {
        GetCurrentJsonValue()[name.data()] = TransferValue(data);
    }

    bool BeginTransfer() final;
    bool EndTransfer() final;
private:
    template<typename T>
        requires(std::is_integral_v<T> || std::is_same_v<T, bool> || std::is_floating_point_v<T> ||
                 std::is_same_v<T, std::string>)
    static Json::Value TransferValue(const T &data) {
        return data;
    }

    template<JsonWriteDetail::InvokeObjectTransfer T>
    Json::Value TransferValue(T &data) {
        PushJsonValue(Json::ValueType::objectValue);
        data.Transfer(*this);
        return PopJsonValue();
    }

    template<JsonWriteDetail::IsSTLContainer T>
    Json::Value TransferValue(T &data) {
        Json::Value value = Json::ValueType::arrayValue;
        for (auto &item : data) {
            value.append(TransferValue(item));
        }
        return value;
    }

    template<typename Key, typename Value>
    Json::Value TransferValue(const std::pair<Key, Value> &pair) {
        PushJsonValue(Json::ValueType::objectValue);
        GetCurrentJsonValue()["Key"] = TransferValue(pair.first);
        GetCurrentJsonValue()["Value"] = TransferValue(pair.second);
        return PopJsonValue();
    }

    void PushJsonValue(Json::ValueType type = Json::ValueType::nullValue);
    auto PopJsonValue() -> Json::Value;
    auto GetCurrentJsonValue() -> Json::Value &;
private:
    std::vector<Json::Value> _stack;
};

static_assert(TransferContextConcept<TransferJsonWriter>);