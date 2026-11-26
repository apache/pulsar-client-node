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
#include <future>

static const std::string PARAM_TLS_CERT = "certificatePath";
static const std::string PARAM_TLS_KEY = "privateKeyPath";
static const std::string PARAM_TOKEN = "token";
static const std::string PARAM_USERNAME = "username";
static const std::string PARAM_PASSWORD = "password";

struct TokenSupplierProxyData : std::enable_shared_from_this<TokenSupplierProxyData> {
  std::promise<std::string> result;
  bool completed = false;

  // Completion and cancellation both run on the JS thread.
  void Complete(std::string token) {
    if (!completed) {
      completed = true;
      result.set_value(std::move(token));
    }
  }
};

void TokenSupplierProxy(Napi::Env env, Napi::Function jsCallback, TokenSupplierProxyData *data) {
  auto request = data->shared_from_this();
  Napi::Value ret = jsCallback.Call({});
  if (ret.IsPromise()) {
    Napi::Promise promise = ret.As<Napi::Promise>();
    Napi::Value thenValue = promise.Get("then");
    if (thenValue.IsFunction()) {
      Napi::Function then = thenValue.As<Napi::Function>();
      Napi::Function callback = Napi::Function::New(env, [request](const Napi::CallbackInfo &info) {
        Napi::Value value = info[0];
        request->Complete(value.IsString() ? value.ToString().Utf8Value() : std::string());
      });
      then.Call(promise, {callback});
      return;
    }
  }
  request->Complete(ret.IsString() ? ret.ToString().Utf8Value() : std::string());
}

char *TokenSupplier(void *ctx) {
  auto tokenSupplierCallback = static_cast<TokenSupplierCallback *>(ctx)->shared_from_this();
  auto request = std::make_shared<TokenSupplierProxyData>();
  auto future = request->result.get_future();

  {
    std::lock_guard<std::mutex> lock(tokenSupplierCallback->mutex);
    if (tokenSupplierCallback->finalized || tokenSupplierCallback->callback.Acquire() != napi_ok) {
      return static_cast<char *>(calloc(1, 1));
    }

    tokenSupplierCallback->pending.insert(request);
    napi_status status = tokenSupplierCallback->callback.BlockingCall(request.get(), TokenSupplierProxy);
    if (status != napi_closing) {
      tokenSupplierCallback->callback.Release();
    }
    if (status != napi_ok) {
      tokenSupplierCallback->pending.erase(request);
      return static_cast<char *>(calloc(1, 1));
    }
  }

  std::string value = future.get();
  {
    std::lock_guard<std::mutex> lock(tokenSupplierCallback->mutex);
    tokenSupplierCallback->pending.erase(request);
  }
  char *token = (char *)malloc(value.size() + 1);
  strcpy(token, value.c_str());
  return token;
}

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
    : Napi::ObjectWrap<Authentication>(info), cAuthentication(nullptr), tokenSupplier(nullptr) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  if (info.Length() < 1 || !info[0].IsString() || info[0].ToString().Utf8Value().empty()) {
    Napi::Error::New(env, "Authentication method is not specified").ThrowAsJavaScriptException();
    return;
  }

  std::string authMethod = info[0].ToString().Utf8Value();

  if (authMethod == "tls" || authMethod == "token" || authMethod == "basic") {
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
      if (obj.Has(PARAM_TOKEN)) {
        if (obj.Get(PARAM_TOKEN).IsString()) {
          this->cAuthentication =
              pulsar_authentication_token_create(obj.Get(PARAM_TOKEN).ToString().Utf8Value().c_str());
          return;
        }

        if (obj.Get(PARAM_TOKEN).IsFunction()) {
          this->tokenSupplier = std::make_shared<TokenSupplierCallback>();
          std::shared_ptr<TokenSupplierCallback> tokenSupplier = this->tokenSupplier;
          // An unbounded queue keeps BlockingCall from waiting while the mutex is held.
          Napi::ThreadSafeFunction callback =
              Napi::ThreadSafeFunction::New(obj.Env(), obj.Get(PARAM_TOKEN).As<Napi::Function>(),
                                            "Token Supplier Callback", 0, 1, [tokenSupplier](Napi::Env) {
                                              std::lock_guard<std::mutex> lock(tokenSupplier->mutex);
                                              tokenSupplier->finalized = true;
                                              for (const auto &request : tokenSupplier->pending) {
                                                request->Complete(std::string());
                                              }
                                              tokenSupplier->pending.clear();
                                            });
          this->tokenSupplier->callback = std::move(callback);
          this->tokenSupplier->callback.Unref(env);

          this->cAuthentication =
              pulsar_authentication_token_create_with_supplier(&TokenSupplier, this->tokenSupplier.get());
          return;
        }
      }

      Napi::Error::New(env, "Missing required parameter").ThrowAsJavaScriptException();
      return;
    } else if (authMethod == "basic") {
      if (!obj.Has(PARAM_USERNAME) || !obj.Get(PARAM_USERNAME).IsString() || !obj.Has(PARAM_PASSWORD) ||
          !obj.Get(PARAM_PASSWORD).IsString()) {
        Napi::Error::New(env, "Missing required parameter").ThrowAsJavaScriptException();
        return;
      }
      this->cAuthentication =
          pulsar_authentication_basic_create(obj.Get(PARAM_USERNAME).ToString().Utf8Value().c_str(),
                                             obj.Get(PARAM_PASSWORD).ToString().Utf8Value().c_str());
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
  if (this->tokenSupplier != nullptr) {
    std::lock_guard<std::mutex> lock(this->tokenSupplier->mutex);
    if (!this->tokenSupplier->finalized) {
      this->tokenSupplier->callback.Release();
    }
  }
  if (this->cAuthentication != nullptr) {
    pulsar_authentication_free(this->cAuthentication);
  }
}

pulsar_authentication_t *Authentication::GetCAuthentication() { return this->cAuthentication; }
