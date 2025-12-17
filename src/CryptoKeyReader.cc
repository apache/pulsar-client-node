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
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

class CryptoKeyReaderWrapper : public pulsar::CryptoKeyReader {
 public:
  CryptoKeyReaderWrapper(const std::map<std::string, std::string>& publicKeys,
                         const std::map<std::string, std::string>& privateKeys)
      : publicKeys_(publicKeys), privateKeys_(privateKeys) {}

  pulsar::Result getPublicKey(const std::string& keyName, std::map<std::string, std::string>& metadata,
                              pulsar::EncryptionKeyInfo& encKeyInfo) const override {
    auto it = publicKeys_.find(keyName);
    if (it != publicKeys_.end()) {
      return readFile(it->second, encKeyInfo);
    }
    return pulsar::Result::ResultCryptoError;
  }

  pulsar::Result getPrivateKey(const std::string& keyName, std::map<std::string, std::string>& metadata,
                               pulsar::EncryptionKeyInfo& encKeyInfo) const override {
    auto it = privateKeys_.find(keyName);
    if (it != privateKeys_.end()) {
      return readFile(it->second, encKeyInfo);
    }
    return pulsar::Result::ResultCryptoError;
  }

 private:
  std::map<std::string, std::string> publicKeys_;
  std::map<std::string, std::string> privateKeys_;

  pulsar::Result readFile(const std::string& path, pulsar::EncryptionKeyInfo& encKeyInfo) const {
    std::ifstream file(path, std::ios::binary);
    if (file) {
      std::stringstream buffer;
      buffer << file.rdbuf();
      encKeyInfo.setKey(buffer.str());
      return pulsar::Result::ResultOk;
    }
    return pulsar::Result::ResultCryptoError;
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

CryptoKeyReader::CryptoKeyReader(const Napi::CallbackInfo& info) : Napi::ObjectWrap<CryptoKeyReader>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  std::map<std::string, std::string> publicKeys;
  std::map<std::string, std::string> privateKeys;

  if (info.Length() > 0 && info[0].IsObject()) {
    Napi::Object config = info[0].As<Napi::Object>();

    if (config.Has("publicKeys") && config.Get("publicKeys").IsObject()) {
      Napi::Object pkObj = config.Get("publicKeys").As<Napi::Object>();
      Napi::Array keys = pkObj.GetPropertyNames();
      for (uint32_t i = 0; i < keys.Length(); ++i) {
        std::string key = keys.Get(i).ToString().Utf8Value();
        std::string path = pkObj.Get(key).ToString().Utf8Value();
        publicKeys[key] = path;
      }
    }

    if (config.Has("privateKeys") && config.Get("privateKeys").IsObject()) {
      Napi::Object pkObj = config.Get("privateKeys").As<Napi::Object>();
      Napi::Array keys = pkObj.GetPropertyNames();
      for (uint32_t i = 0; i < keys.Length(); ++i) {
        std::string key = keys.Get(i).ToString().Utf8Value();
        std::string path = pkObj.Get(key).ToString().Utf8Value();
        privateKeys[key] = path;
      }
    }
  }

  this->cCryptoKeyReader = std::make_shared<CryptoKeyReaderWrapper>(publicKeys, privateKeys);
}

CryptoKeyReader::~CryptoKeyReader() {}

std::shared_ptr<pulsar::CryptoKeyReader> CryptoKeyReader::GetCCryptoKeyReader() {
  return this->cCryptoKeyReader;
}
