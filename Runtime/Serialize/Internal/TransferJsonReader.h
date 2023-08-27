#pragma once
#include <json/json.h>

namespace JsonReaderDetail {

template<typename T>
concept IsStdVectorOrList = IsStdVector<T>::value || IsStdList<T>::value;

template<typename T>
concept IsJsonNumber = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept IsSetContainer = IsStdSet<T>::value || IsStdMultiSet<T>::value || IsStdUnorderedSet<T>::value ||
                         IsStdUnorderedMultiSet<T>::value;

template<typename T>
concept IsMapContainer = IsStdMap<T>::value || IsStdMultiMap<T>::value || IsStdUnorderedMap<T>::value ||
                         IsStdUnorderedMultiMap<T>::value;

template<typename T>
concept IsContainer = IsStdVectorOrList<T> || IsSetContainer<T> || IsMapContainer<T>;

template<typename T>
concept InvokeObjectTransfer = requires(T a) {
	a.Transfer(std::declval<TransferJsonReader &>());
};

}    // namespace JsonReaderDetail

class TransferJsonReader : public TransferBase, public ITransferReader {
public:
    using TransferBase::TransferBase;
    void TransferVersion(std::string_view name, int version) final;
    void Transfer(std::string_view name, bool &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, uint8 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, uint16 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, uint32 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, uint64 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, int8 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, int16 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, int32 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, int64 &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, float &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, double &data) final {
        TransferValueWithName(name, data);
    }
    void Transfer(std::string_view name, std::string &data) final {
        TransferValueWithName(name, data);
    }

    template<JsonReaderDetail::InvokeObjectTransfer T>
    void Transfer(std::string_view name, T &data) {
        TransferValueWithName(name, data);
    }

    bool BeginTransfer() final;
    bool EndTransfer() final;

    template<JsonReaderDetail::IsContainer T>
    void Transfer(std::string_view name, T &data) {
        data.clear();
        if (!PushJsonValue(name)) {
            return;
        }
        TransferValue(data);
        PopJsonValue();
    }

    void PushJsonValue(Json::Value &value);
    bool PushJsonValue(std::string_view name);
    void PopJsonValue();
    auto GetCurrentJsonValue() const -> Json::Value &;
private:
    template<typename T>
    void TransferValueWithName(std::string_view name, T &data) {
        if (!PushJsonValue(name)) {
            return;
        }
        TransferValue(data);
        PopJsonValue();
    }

    bool TransferValue(bool &data) {
        Json::Value &value = GetCurrentJsonValue();
        if (value.isBool()) {
            data = value.asBool();
            return true;
        }
        return false;
    }

    bool TransferValue(std::string &data) {
        Json::Value &value = GetCurrentJsonValue();
        if (value.isString()) {
            data = value.asCString();
            return true;
        }
        return false;
    }

    template<JsonReaderDetail::IsJsonNumber T>
    bool TransferValue(T &data) {
        Json::Value &value = GetCurrentJsonValue();
        if (value.isNumeric()) {
            data = static_cast<T>(value.asDouble());
            return true;
        }
        return false;
    }

    template<JsonReaderDetail::IsStdVectorOrList T>
    bool TransferValue(T &data) {
        Json::Value &value = GetCurrentJsonValue();
        if (!value.isArray()) {
            return false;
        }
        for (Json::Value &element : value) {
            PushJsonValue(element);
            TransferValue(data.emplace_back());
            PopJsonValue();
        }
        return true;
    }

    template<JsonReaderDetail::IsSetContainer T>
    bool TransferValue(T &data) {
        Json::Value &value = GetCurrentJsonValue();
        if (!value.isArray()) {
            return false;
        }

        for (auto &element : value) {
            PushJsonValue(element);
            typename T::key_type key{};
            if (TransferValue(key)) {
                data.insert(key);
            }
            PopJsonValue();
        }
        return true;
    }

    template<JsonReaderDetail::IsMapContainer T>
    bool TransferValue(T &data) {
        Json::Value &value = GetCurrentJsonValue();
	    if (!value.isArray()) {
            return false;
        }

        for (auto &element : value) {
            PushJsonValue(element);
            using KeyType = typename GetStdMapPair<T>::KeyType;
            using ValueType = typename GetStdMapPair<T>::ValueType;
            using PairType = typename GetStdMapPair<T>::PairType;
            std::pair<KeyType, ValueType> pair;
            if (TransferValue(pair)) {
                data.insert(PairType(pair.first, pair.second));
            }
            PopJsonValue();
        }
        return true;
    }

    template<typename Key, typename Value>
    bool TransferValue(std::pair<Key, Value> &data) {
        Json::Value &value = GetCurrentJsonValue();
	    if (!value.isObject()) {
		    return false;
	    }

        if (!PushJsonValue("Key")) {
	        return false;
        }
        bool ret = TransferValue(data.first);
        PopJsonValue();

        if (ret && !PushJsonValue("Value")) {
	        return false;
        }
        ret = TransferValue(data.second);
        PopJsonValue();
        return ret;
    }

    template<JsonReaderDetail::InvokeObjectTransfer T>
    bool TransferValue(T &data) {
		data.Transfer(*this);
        return true;
    }
private:
    Json::Value _root;
    std::vector<Json::Value *> _stack;
};
