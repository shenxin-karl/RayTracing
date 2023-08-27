#include "Serialize/Transfer.hpp"
#include "Foundation/Exception.h"
#include <fstream>

void TransferJsonReader::TransferVersion(std::string_view name, int version) {
    _currentVersion = GetVersion(name);
}

bool TransferJsonReader::BeginTransfer() {
    std::ifstream input(_sourceFilePath);
    if (!input.is_open()) {
	    return false;
    }

    JSONCPP_STRING errs;
    Json::CharReaderBuilder builder;
    if (!parseFromStream(builder, input, &_root, &errs)) {
        return false;
    }
    _stack.push_back(&_root);

    if (PushJsonValue("__VersionMap")) {
	    Json::Value &versionObject = GetCurrentJsonValue();
	    for (Json::Value &element : versionObject) {
	        std::string key = element["Name"].asCString();
	        int version = element["Version"].asInt();
	        _versionMap[key] = version;
	    }
	    PopJsonValue();
    }
    return true;
}

bool TransferJsonReader::EndTransfer() {
    _stack.clear();
    _root = Json::ValueType::nullValue;
	TransferBase::EndTransfer();
    return true;
}

void TransferJsonReader::PushJsonValue(Json::Value &value) {
    _stack.push_back(&value);
}

bool TransferJsonReader::PushJsonValue(std::string_view name) {
    Json::Value &value = GetCurrentJsonValue();
    if (value.isMember(name.data())) {
        _stack.push_back(&value[name.data()]);
        return true;
    }
    return false;
}

void TransferJsonReader::PopJsonValue() {
    Assert(_stack.size() >= 2);
    _stack.pop_back();
}

auto TransferJsonReader::GetCurrentJsonValue() const -> Json::Value & {
    return *_stack.back();
}
