//
//  any.cpp
//  Ikadenwa
//
//  Created by omochimetaru on 2016/02/10.
//  Copyright © 2016年 omochimetaru. All rights reserved.
//

#include "any.h"
#include "env.h"

#include "json.h"

namespace nwr {
    Any::Any(): type_(Type::Null), value_(nullptr) {}
    Any::Any(const Any & copy): type_(Type::Null), value_(nullptr) {
        *this = copy;
    }
    Any::Any(Any && move): type_(Type::Null), value_(nullptr) {
        *this = move;
    }
    
    Any::Any(std::nullptr_t value): type_(Type::Null), value_(nullptr) {}
    
    Any::Any(bool value): type_(Type::Boolean), value_(std::make_shared<bool>(value)) {}
    
    Any::Any(int value): Any(static_cast<double>(value)){}
    Any::Any(double value): type_(Type::Number), value_(std::make_shared<double>(value)) {}
    
    Any::Any(const char * value): Any(std::string(value)) {};
    Any::Any(const std::string & value): type_(Type::String), value_(std::make_shared<std::string>(value)) {}
    
    Any::Any(const Data & value): Any(std::make_shared<Data>(value)){}
    Any::Any(const DataPtr & value): type_(Type::Data), value_(value) {}
    
    Any::Any(const std::vector<Any> & value):
    type_(Type::Array), value_(std::make_shared<std::vector<Any>>(value)) {}

    Any::Any(const std::map<std::string, Any> & value):
    type_(Type::Dictionary), value_(std::make_shared<std::map<std::string, Any>>(value)) {}
    
    Any::Type Any::type() const {
        return type_;
    }
    
    int Any::count() const {
        switch (type()) {
            case Type::Array:
                return static_cast<int>(AsArray()->size());
            case Type::Dictionary:
                return static_cast<int>(AsDictionary()->size());
            default:
                return 0;
        }
    }
    
    std::vector<std::string> Any::keys() const {
        if (type() == Type::Dictionary) {
            std::vector<std::string> r;
            for (auto entry : *inner_dictionary()) {
                r.push_back(entry.first);
            }
            return r;
        } else {
            return std::vector<std::string>();
        }
    }
    
    Optional<bool> Any::AsBoolean() const {
        if (type() == Type::Boolean) {
            return Some(*std::static_pointer_cast<bool>(value_));
        } else {
            return None();
        }
    }
    Optional<int> Any::AsInt() const {
        return AsDouble().Map<int>([](double x){ return static_cast<int>(x); });
    }
    Optional<double> Any::AsDouble() const {
        if (type() == Type::Number) {
            return Some(*std::static_pointer_cast<double>(value_));
        } else {
            return None();
        }
    }
    Optional<std::string> Any::AsString() const {
        if (type() == Type::String) {
            return Some(*std::static_pointer_cast<std::string>(value_));
        } else {
            return None();
        }
    }
    Optional<DataPtr> Any::AsData() const {
        if (type() == Type::Data) {
            return Some(std::static_pointer_cast<Data>(value_));
        } else {
            return None();
        }
    }
    Optional<std::vector<Any>> Any::AsArray() const {
        auto array = inner_array();
        return array ? Some(*array) : None();
    }
    Optional<std::map<std::string, Any>> Any::AsDictionary() const {
        auto dict = inner_dictionary();
        return dict ? Some(*dict) : None();
    }
    
    Any & Any::operator= (const Any & copy) {
        type_ = copy.type_;
        
        switch (type_) {
            case Type::Null:
                value_ = nullptr;
                break;
            case Type::Boolean:
                value_ = std::make_shared<bool>(*copy.AsBoolean());
                break;
            case Type::Number:
                value_ = std::make_shared<double>(*copy.AsDouble());
                break;
            case Type::String:
                value_ = std::make_shared<std::string>(*copy.AsString());
                break;
            case Type::Data:
            case Type::Array:
            case Type::Dictionary:
                value_ = copy.value_;
                break;
        }
        
        return *this;
    }
    Any & Any::operator= (Any && move) {
        type_ = move.type_;
        value_ = move.value_;
        move.type_ = Type::Null;
        move.value_ = nullptr;
        return *this;
    }
    
    Any Any::GetAt(int index) const {
        auto array = inner_array();
        if (array) {
            if (0 <= index && index < array->size()) {
                return array->at(index);
            }
        }
        return nullptr;
    }
    void Any::SetAt(int index, const Any & value) {
        auto array = inner_array();
        if (!array) { Fatal("not array"); }
        if (index < 0) { Fatal("invalid index"); }
        if (array->size() <= index) {
            array->resize(index + 1);
        }
        array->at(index) = value;
    }
    Any Any::GetAt(const std::string & key) const {
        auto dict = inner_dictionary();
        if (dict) {
            if (dict->find(key) != dict->end()) {
                return dict->at(key);
            }
        }
        return nullptr;
    }
    void Any::SetAt(const std::string & key, const Any & value) {
        auto dict = inner_dictionary();
        if (!dict) { Fatal("not dictionary"); }
        dict->at(key) = value;
    }
    
    Any Any::FromJson(const Json::Value & json) {
        switch (json.type()) {
            case Json::nullValue:
                return Any(nullptr);
            case Json::intValue:
                return Any(json.asInt());
            case Json::uintValue:
                return Any(static_cast<double>(json.asUInt()));
            case Json::realValue:
                return Any(json.asDouble());
            case Json::stringValue:
                return Any(json.asString());
            case Json::booleanValue:
                return Any(json.asBool());
            case Json::arrayValue: {
                std::vector<Any> array;
                for (int i = 0; i < json.size(); i++) {
                    array.push_back(FromJson(json[i]));
                }
                return Any(array);
            }
            case Json::objectValue: {
                std::map<std::string, Any> dict;
                for (auto key : json.getMemberNames()) {
                    dict[key] = FromJson(json.get(key, Json::Value()));
                }
                return Any(dict);
            }
        }
    }
    
    std::shared_ptr<Json::Value> Any::ToJson() const {
        switch (type()) {
            case Type::Null:
                return std::make_shared<Json::Value>();
            case Type::Boolean:
                return std::make_shared<Json::Value>(*AsBoolean());
            case Type::Number:
                return std::make_shared<Json::Value>(*AsDouble());
            case Type::String:
                return std::make_shared<Json::Value>(*AsString());
            case Type::Data:
                return std::make_shared<Json::Value>(DataFormat(**AsData()));
            case Type::Array: {
                auto json = std::make_shared<Json::Value>(Json::arrayValue);
                for (int i = 0; i < count(); i++) {
                    json->append(*GetAt(i).ToJson());
                }
                return json;
            }
            case Type::Dictionary: {
                auto json = std::make_shared<Json::Value>(Json::objectValue);
                for (auto key : keys()) {
                    (*json)[key] = *GetAt(key).ToJson();
                }
                return json;
            }
        }
    }
    
    std::shared_ptr<std::vector<Any>> Any::inner_array() const {
        if (type() == Type::Array) {
            return std::static_pointer_cast<std::vector<Any>>(value_);
        } else {
            return nullptr;
        }
    }
    std::shared_ptr<std::map<std::string, Any>> Any::inner_dictionary() const {
        if (type() == Type::Dictionary) {
            return std::static_pointer_cast<std::map<std::string, Any>>(value_);
        } else {
            return nullptr;
        }
    }
}