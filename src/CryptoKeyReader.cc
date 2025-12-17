/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "CryptoKeyReader.h"
#include <pulsar/Result.h>
#include <pulsar/EncryptionKeyInfo.h>
#include <thread>
#include <future>
#include <iostream>

class CryptoKeyReaderWrapper : public pulsar::CryptoKeyReader {
 public:
  CryptoKeyReaderWrapper(const Napi::Object& jsObject) : mainThreadId_(std::this_thread::get_id()) {
    jsObject_.Reset(jsObject, 1);
    tsfn_ = Napi::ThreadSafeFunction::New(
        jsObject.Env(), Napi::Function::New(jsObject.Env(), [](const Napi::CallbackInfo& info) {}), jsObject,
        "CryptoKeyReader", 0, 1);
  }

  ~CryptoKeyReaderWrapper() { tsfn_.Release(); }

  pulsar::Result getPublicKey(const std::string& keyName, std::map<std::string, std::string>& metadata,
                              pulsar::EncryptionKeyInfo& encKeyInfo) const override {
    return executeCallback("getPublicKey", keyName, metadata, encKeyInfo);
  }

  pulsar::Result getPrivateKey(const std::string& keyName, std::map<std::string, std::string>& metadata,
                               pulsar::EncryptionKeyInfo& encKeyInfo) const override {
    return executeCallback("getPrivateKey", keyName, metadata, encKeyInfo);
  }

 private:
  mutable Napi::ObjectReference jsObject_;
  Napi::ThreadSafeFunction tsfn_;
  std::thread::id mainThreadId_;

  struct CallbackData {
    std::string method;
    std::string keyName;
    std::map<std::string, std::string> metadata;
    std::shared_ptr<std::promise<pulsar::Result>> promise;
    pulsar::EncryptionKeyInfo* encKeyInfo;
    Napi::ObjectReference* jsObjectRef;
  };

  static void parseEncryptionKeyInfo(const Napi::Object& obj, pulsar::EncryptionKeyInfo& info) {
    if (obj.Has("key") && obj.Get("key").IsBuffer()) {
      Napi::Buffer<char> keyBuf = obj.Get("key").As<Napi::Buffer<char>>();
      info.setKey(std::string(keyBuf.Data(), keyBuf.Length()));
    }
    if (obj.Has("metadata") && obj.Get("metadata").IsObject()) {
      std::map<std::string, std::string> metadata;
      Napi::Object metaObj = obj.Get("metadata").As<Napi::Object>();
      Napi::Array keys = metaObj.GetPropertyNames();
      for (uint32_t i = 0; i < keys.Length(); i++) {
        std::string k = keys.Get(i).ToString().Utf8Value();
        std::string v = metaObj.Get(k).ToString().Utf8Value();
        metadata[k] = v;
      }
      info.setMetadata(metadata);
    }
  }

  pulsar::Result executeCallback(const std::string& method, const std::string& keyName,
                                 std::map<std::string, std::string>& metadata,
                                 pulsar::EncryptionKeyInfo& encKeyInfo) const {
    if (std::this_thread::get_id() == mainThreadId_) {
      Napi::Env env = jsObject_.Env();
      Napi::HandleScope scope(env);

      if (jsObject_.IsEmpty()) {
        return pulsar::Result::ResultCryptoError;
      }
      Napi::Object obj = jsObject_.Value();

      if (!obj.Has(method)) {
        return pulsar::Result::ResultCryptoError;
      }
      Napi::Value funcVal = obj.Get(method);
      if (!funcVal.IsFunction()) {
        return pulsar::Result::ResultCryptoError;
      }
      Napi::Function func = funcVal.As<Napi::Function>();

      Napi::Object metadataObj = Napi::Object::New(env);
      for (const auto& kv : metadata) {
        metadataObj.Set(kv.first, kv.second);
      }

      try {
        Napi::Value result = func.Call(obj, {Napi::String::New(env, keyName), metadataObj});
        if (result.IsObject()) {
          parseEncryptionKeyInfo(result.As<Napi::Object>(), encKeyInfo);
          return pulsar::Result::ResultOk;
        }
      } catch (const Napi::Error& e) {
        return pulsar::Result::ResultCryptoError;
      }
      return pulsar::Result::ResultCryptoError;
    } else {
      auto promise = std::make_shared<std::promise<pulsar::Result>>();
      auto future = promise->get_future();

      auto* data = new CallbackData{method, keyName, metadata, promise, &encKeyInfo, &jsObject_};

      napi_status status =
          tsfn_.BlockingCall(data, [](Napi::Env env, Napi::Function jsCallback, void* context) {
            CallbackData* data = static_cast<CallbackData*>(context);

            Napi::HandleScope scope(env);
            Napi::Object obj = data->jsObjectRef->Value();

            pulsar::Result res = pulsar::Result::ResultCryptoError;
            if (obj.Has(data->method) && obj.Get(data->method).IsFunction()) {
              Napi::Function func = obj.Get(data->method).As<Napi::Function>();
              Napi::Object metadataObj = Napi::Object::New(env);
              for (const auto& kv : data->metadata) {
                metadataObj.Set(kv.first, kv.second);
              }

              try {
                Napi::Value result = func.Call(obj, {Napi::String::New(env, data->keyName), metadataObj});
                if (result.IsObject()) {
                  parseEncryptionKeyInfo(result.As<Napi::Object>(), *data->encKeyInfo);
                  res = pulsar::Result::ResultOk;
                }
              } catch (...) {
                res = pulsar::Result::ResultCryptoError;
              }
            }

            data->promise->set_value(res);
            delete data;
          });

      if (status != napi_ok) {
        delete data;
        return pulsar::Result::ResultCryptoError;
      }

      future.wait();
      return future.get();
    }
  }
};

Napi::FunctionReference CryptoKeyReader::constructor;

void CryptoKeyReader::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "CryptoKeyReader", {});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("CryptoKeyReader", func);
}

CryptoKeyReader::CryptoKeyReader(const Napi::CallbackInfo& info) : Napi::ObjectWrap<CryptoKeyReader>(info) {}

CryptoKeyReader::~CryptoKeyReader() {}

std::shared_ptr<pulsar::CryptoKeyReader> CryptoKeyReader::GetCCryptoKeyReader() {
  return std::make_shared<CryptoKeyReaderWrapper>(Value());
}
