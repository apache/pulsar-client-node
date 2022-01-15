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

#include "Authentication.h"

static const std::string PARAM_TLS_CERT = "certificatePath";
static const std::string PARAM_TLS_KEY = "privateKeyPath";
static const std::string PARAM_TOKEN = "token";

Napi::FunctionReference Authentication::constructor;

Napi::Object Authentication::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Authentication", {});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Authentication", func);
  return exports;
}

Authentication::Authentication(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<Authentication>(info), cAuthentication(nullptr) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  if (info.Length() < 1 || !info[0].IsString() || info[0].ToString().Utf8Value().empty()) {
    Napi::Error::New(env, "Authentication method is not specified").ThrowAsJavaScriptException();
    return;
  }

  std::string authMethod = info[0].ToString().Utf8Value();

  if (authMethod == "tls" || authMethod == "token") {
    if (info.Length() < 2 || !info[1].IsObject()) {
      Napi::Error::New(env, "Authentication parameter must be a object").ThrowAsJavaScriptException();
      return;
    }

    Napi::Object obj = info[1].ToObject();

    if (authMethod == "tls") {
      if (!obj.Has(PARAM_TLS_CERT) || !obj.Get(PARAM_TLS_CERT).IsString() || !obj.Has(PARAM_TLS_KEY) ||
          !obj.Get(PARAM_TLS_KEY).IsString()) {
        Napi::Error::New(env, "Missing required parameter").ThrowAsJavaScriptException();
        return;
      }
      this->cAuthentication =
          pulsar_authentication_tls_create(obj.Get(PARAM_TLS_CERT).ToString().Utf8Value().c_str(),
                                           obj.Get(PARAM_TLS_KEY).ToString().Utf8Value().c_str());
    } else if (authMethod == "token") {
      if (!obj.Has(PARAM_TOKEN) || !obj.Get(PARAM_TOKEN).IsString()) {
        Napi::Error::New(env, "Missing required parameter").ThrowAsJavaScriptException();
        return;
      }
      this->cAuthentication =
          pulsar_authentication_token_create(obj.Get(PARAM_TOKEN).ToString().Utf8Value().c_str());
    }
  } else if (authMethod == "athenz") {
    if (info.Length() < 2 || !info[1].IsString()) {
      Napi::Error::New(env, "Authentication parameter must be a JSON string").ThrowAsJavaScriptException();
      return;
    }
    this->cAuthentication = pulsar_authentication_athenz_create(info[1].ToString().Utf8Value().c_str());
  } else if (authMethod == "oauth2") {
    if (info.Length() < 2 || !info[1].IsString()) {
      Napi::Error::New(env, "Authentication parameter must be a JSON string for oauth2")
          .ThrowAsJavaScriptException();
      return;
    }
    this->cAuthentication = pulsar_authentication_oauth2_create(info[1].ToString().Utf8Value().c_str());
  } else {
    Napi::Error::New(env, "Unsupported authentication method").ThrowAsJavaScriptException();
    return;
  }
}

Authentication::~Authentication() {
  if (this->cAuthentication != nullptr) {
    pulsar_authentication_free(this->cAuthentication);
  }
}

pulsar_authentication_t *Authentication::GetCAuthentication() { return this->cAuthentication; }
