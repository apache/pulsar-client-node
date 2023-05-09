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
#include "Client.h"
#include "Consumer.h"
#include "Producer.h"
#include "Reader.h"
#include "ThreadSafeDeferred.h"
#include <pulsar/c/client.h>
#include <pulsar/c/client_configuration.h>
#include <pulsar/c/result.h>

static const std::string CFG_SERVICE_URL = "serviceUrl";
static const std::string CFG_AUTH = "authentication";
static const std::string CFG_AUTH_PROP = "binding";
static const std::string CFG_OP_TIMEOUT = "operationTimeoutSeconds";
static const std::string CFG_IO_THREADS = "ioThreads";
static const std::string CFG_LISTENER_THREADS = "messageListenerThreads";
static const std::string CFG_CONCURRENT_LOOKUP = "concurrentLookupRequest";
static const std::string CFG_USE_TLS = "useTls";
static const std::string CFG_TLS_TRUST_CERT = "tlsTrustCertsFilePath";
static const std::string CFG_TLS_VALIDATE_HOSTNAME = "tlsValidateHostname";
static const std::string CFG_TLS_ALLOW_INSECURE = "tlsAllowInsecureConnection";
static const std::string CFG_STATS_INTERVAL = "statsIntervalInSeconds";
static const std::string CFG_LOG = "log";

LogCallback *Client::logCallback = nullptr;

void Client::SetLogHandler(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  Napi::Value jsFunction = info[0];

  if (jsFunction.IsNull()) {
    if (Client::logCallback != nullptr) {
      Client::logCallback->callback.Release();
      delete Client::logCallback;
      Client::logCallback = nullptr;
    }
  } else if (jsFunction.IsFunction()) {
    Napi::ThreadSafeFunction logFunction =
        Napi::ThreadSafeFunction::New(env, jsFunction.As<Napi::Function>(), "Pulsar Logging", 0, 1);
    logFunction.Unref(env);
    if (Client::logCallback != nullptr) {
      Client::logCallback->callback.Release();
    } else {
      Client::logCallback = new LogCallback();
    }
    Client::logCallback->callback = std::move(logFunction);
  } else {
    Napi::Error::New(env, "Client log handler must be a function or null").ThrowAsJavaScriptException();
  }
}

Napi::FunctionReference Client::constructor;

Napi::Object Client::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "Client",
      {StaticMethod("setLogHandler", &Client::SetLogHandler),
       InstanceMethod("createProducer", &Client::CreateProducer),
       InstanceMethod("subscribe", &Client::Subscribe), InstanceMethod("createReader", &Client::CreateReader),
       InstanceMethod("getPartitionsForTopic", &Client::GetPartitionsForTopic),
       InstanceMethod("close", &Client::Close)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Client", func);
  return exports;
}

Client::Client(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Client>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  Napi::Object clientConfig = info[0].As<Napi::Object>();

  if (!clientConfig.Has(CFG_SERVICE_URL) || !clientConfig.Get(CFG_SERVICE_URL).IsString() ||
      clientConfig.Get(CFG_SERVICE_URL).ToString().Utf8Value().empty()) {
    Napi::Error::New(env, "Service URL is required and must be specified as a string")
        .ThrowAsJavaScriptException();
    return;
  }
  Napi::String serviceUrl = clientConfig.Get(CFG_SERVICE_URL).ToString();

  this->cClientConfig = std::shared_ptr<pulsar_client_configuration_t>(pulsar_client_configuration_create(),
                                                                       pulsar_client_configuration_free);

  // The logger can only be set once per process, so we will take control of it
  pulsar_client_configuration_set_logger(cClientConfig.get(), &LogMessage, nullptr);

  // log config option should be deprecated in favour of static setLogHandler method
  if (clientConfig.Has(CFG_LOG) && clientConfig.Get(CFG_LOG).IsFunction()) {
    Napi::ThreadSafeFunction logFunction = Napi::ThreadSafeFunction::New(
        env, clientConfig.Get(CFG_LOG).As<Napi::Function>(), "Pulsar Logging", 0, 1);
    logFunction.Unref(env);

    if (Client::logCallback != nullptr) {
      Client::logCallback->callback.Release();
    } else {
      Client::logCallback = new LogCallback();
    }
    Client::logCallback->callback = std::move(logFunction);
  }

  if (clientConfig.Has(CFG_AUTH) && clientConfig.Get(CFG_AUTH).IsObject()) {
    Napi::Object obj = clientConfig.Get(CFG_AUTH).ToObject();
    if (obj.Has(CFG_AUTH_PROP) && obj.Get(CFG_AUTH_PROP).IsObject()) {
      Authentication *auth = Authentication::Unwrap(obj.Get(CFG_AUTH_PROP).ToObject());
      pulsar_client_configuration_set_auth(cClientConfig.get(), auth->GetCAuthentication());
    }
  }

  if (clientConfig.Has(CFG_OP_TIMEOUT) && clientConfig.Get(CFG_OP_TIMEOUT).IsNumber()) {
    int32_t operationTimeoutSeconds = clientConfig.Get(CFG_OP_TIMEOUT).ToNumber().Int32Value();
    if (operationTimeoutSeconds > 0) {
      pulsar_client_configuration_set_operation_timeout_seconds(cClientConfig.get(), operationTimeoutSeconds);
    }
  }

  if (clientConfig.Has(CFG_IO_THREADS) && clientConfig.Get(CFG_IO_THREADS).IsNumber()) {
    int32_t ioThreads = clientConfig.Get(CFG_IO_THREADS).ToNumber().Int32Value();
    if (ioThreads > 0) {
      pulsar_client_configuration_set_io_threads(cClientConfig.get(), ioThreads);
    }
  }

  if (clientConfig.Has(CFG_LISTENER_THREADS) && clientConfig.Get(CFG_LISTENER_THREADS).IsNumber()) {
    int32_t messageListenerThreads = clientConfig.Get(CFG_LISTENER_THREADS).ToNumber().Int32Value();
    if (messageListenerThreads > 0) {
      pulsar_client_configuration_set_message_listener_threads(cClientConfig.get(), messageListenerThreads);
    }
  }

  if (clientConfig.Has(CFG_CONCURRENT_LOOKUP) && clientConfig.Get(CFG_CONCURRENT_LOOKUP).IsNumber()) {
    int32_t concurrentLookupRequest = clientConfig.Get(CFG_CONCURRENT_LOOKUP).ToNumber().Int32Value();
    if (concurrentLookupRequest > 0) {
      pulsar_client_configuration_set_concurrent_lookup_request(cClientConfig.get(), concurrentLookupRequest);
    }
  }

  if (clientConfig.Has(CFG_USE_TLS) && clientConfig.Get(CFG_USE_TLS).IsBoolean()) {
    Napi::Boolean useTls = clientConfig.Get(CFG_USE_TLS).ToBoolean();
    pulsar_client_configuration_set_use_tls(cClientConfig.get(), useTls.Value());
  }

  if (clientConfig.Has(CFG_TLS_TRUST_CERT) && clientConfig.Get(CFG_TLS_TRUST_CERT).IsString()) {
    Napi::String tlsTrustCertsFilePath = clientConfig.Get(CFG_TLS_TRUST_CERT).ToString();
    pulsar_client_configuration_set_tls_trust_certs_file_path(cClientConfig.get(),
                                                              tlsTrustCertsFilePath.Utf8Value().c_str());
  }

  if (clientConfig.Has(CFG_TLS_VALIDATE_HOSTNAME) &&
      clientConfig.Get(CFG_TLS_VALIDATE_HOSTNAME).IsBoolean()) {
    Napi::Boolean tlsValidateHostname = clientConfig.Get(CFG_TLS_VALIDATE_HOSTNAME).ToBoolean();
    pulsar_client_configuration_set_validate_hostname(cClientConfig.get(), tlsValidateHostname.Value());
  }

  if (clientConfig.Has(CFG_TLS_ALLOW_INSECURE) && clientConfig.Get(CFG_TLS_ALLOW_INSECURE).IsBoolean()) {
    Napi::Boolean tlsAllowInsecureConnection = clientConfig.Get(CFG_TLS_ALLOW_INSECURE).ToBoolean();
    pulsar_client_configuration_set_tls_allow_insecure_connection(cClientConfig.get(),
                                                                  tlsAllowInsecureConnection.Value());
  }

  if (clientConfig.Has(CFG_STATS_INTERVAL) && clientConfig.Get(CFG_STATS_INTERVAL).IsNumber()) {
    uint32_t statsIntervalInSeconds = clientConfig.Get(CFG_STATS_INTERVAL).ToNumber().Uint32Value();
    pulsar_client_configuration_set_stats_interval_in_seconds(cClientConfig.get(), statsIntervalInSeconds);
  }

  try {
    this->cClient = std::shared_ptr<pulsar_client_t>(
        pulsar_client_create(serviceUrl.Utf8Value().c_str(), cClientConfig.get()), pulsar_client_free);
  } catch (const std::exception &e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
  }
}

Client::~Client() {}

Napi::Value Client::CreateProducer(const Napi::CallbackInfo &info) {
  return Producer::NewInstance(info, this->cClient);
}
Napi::Value Client::Subscribe(const Napi::CallbackInfo &info) {
  return Consumer::NewInstance(info, this->cClient);
}

Napi::Value Client::CreateReader(const Napi::CallbackInfo &info) {
  return Reader::NewInstance(info, this->cClient);
}

Napi::Value Client::GetPartitionsForTopic(const Napi::CallbackInfo &info) {
  Napi::String topicString = info[0].As<Napi::String>();
  std::string topic = topicString.Utf8Value();
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_client_get_topic_partitions_async(
      this->cClient.get(), topic.c_str(),
      [](pulsar_result result, pulsar_string_list_t *topicList, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result == pulsar_result_Ok && topicList != nullptr) {
          deferred->Resolve([topicList](const Napi::Env env) {
            int listSize = pulsar_string_list_size(topicList);
            Napi::Array jsArray = Napi::Array::New(env, listSize);

            for (int i = 0; i < listSize; i++) {
              const char *str = pulsar_string_list_get(topicList, i);
              jsArray.Set(i, Napi::String::New(env, str));
            }

            return jsArray;
          });
        } else {
          deferred->Reject(std::string("Failed to GetPartitionsForTopic: ") + pulsar_result_str(result));
        }
      },

      ctx);

  return deferred->Promise();
}

void LogMessageProxy(Napi::Env env, Napi::Function jsCallback, struct LogMessage *logMessage) {
  Napi::Number logLevel = Napi::Number::New(env, static_cast<double>(logMessage->level));
  Napi::String file = Napi::String::New(env, logMessage->file);
  Napi::Number line = Napi::Number::New(env, static_cast<double>(logMessage->line));
  Napi::String message = Napi::String::New(env, logMessage->message);

  delete logMessage;
  jsCallback.Call({logLevel, file, line, message});
}

void Client::LogMessage(pulsar_logger_level_t level, const char *file, int line, const char *message,
                        void *ctx) {
  LogCallback *logCallback = Client::logCallback;

  if (logCallback == nullptr) {
    return;
  }

  if (logCallback->callback.Acquire() != napi_ok) {
    return;
  }

  struct LogMessage *logMessage = new struct LogMessage(level, std::string(file), line, std::string(message));

  logCallback->callback.BlockingCall(logMessage, LogMessageProxy);
  logCallback->callback.Release();
}

Napi::Value Client::Close(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_client_close_async(
      this->cClient.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to close client: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}
