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
#include <pulsar/AutoClusterFailover.h>
#include <pulsar/Client.h>
#include <pulsar/ServiceInfo.h>
#include <pulsar/c/authentication.h>
#include <pulsar/c/client.h>
#include <pulsar/c/client_configuration.h>
#include <pulsar/c/result.h>
#include "pulsar/ClientConfiguration.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

static const std::string CFG_SERVICE_URL = "serviceUrl";
static const std::string CFG_SERVICE_URL_PROVIDER = "serviceUrlProvider";
static const std::string CFG_PRIMARY = "primary";
static const std::string CFG_SECONDARY = "secondary";
static const std::string CFG_CHECK_INTERVAL_MS = "checkIntervalMs";
static const std::string CFG_FAILOVER_THRESHOLD = "failoverThreshold";
static const std::string CFG_SWITCH_BACK_THRESHOLD = "switchBackThreshold";
static const std::string CFG_AUTH = "authentication";
static const std::string CFG_AUTH_PROP = "binding";
static const std::string CFG_OP_TIMEOUT = "operationTimeoutSeconds";
static const std::string CFG_IO_THREADS = "ioThreads";
static const std::string CFG_LISTENER_THREADS = "messageListenerThreads";
static const std::string CFG_CONCURRENT_LOOKUP = "concurrentLookupRequest";
static const std::string CFG_TLS_TRUST_CERT = "tlsTrustCertsFilePath";
static const std::string CFG_TLS_VALIDATE_HOSTNAME = "tlsValidateHostname";
static const std::string CFG_TLS_ALLOW_INSECURE = "tlsAllowInsecureConnection";
static const std::string CFG_TLS_CERT_FILE = "tlsCertificateFilePath";
static const std::string CFG_TLS_PRIVATE_KEY_FILE = "tlsPrivateKeyFilePath";
static const std::string CFG_STATS_INTERVAL = "statsIntervalInSeconds";
static const std::string CFG_LOG = "log";
static const std::string CFG_LOG_LEVEL = "logLevel";
static const std::string CFG_LISTENER_NAME = "listenerName";
static const std::string CFG_CONNECTION_TIMEOUT = "connectionTimeoutMs";

LogCallback *Client::logCallback = nullptr;

struct _pulsar_client_configuration {
  pulsar::ClientConfiguration conf;
};

struct _pulsar_client {
  std::unique_ptr<pulsar::Client> client;
};

struct _pulsar_authentication {
  pulsar::AuthenticationPtr auth;
};

static bool IsPresent(const Napi::Value &value) { return !value.IsUndefined() && !value.IsNull(); }

static std::optional<pulsar::AuthenticationPtr> BuildAuthenticationPtr(
    const Napi::Object &authObject, std::vector<Napi::ObjectReference> &authRefs) {
  Napi::Env env = authObject.Env();

  if (!authObject.Has(CFG_AUTH_PROP) || !authObject.Get(CFG_AUTH_PROP).IsObject()) {
    Napi::Error::New(env, "Authentication must be a Pulsar authentication object")
        .ThrowAsJavaScriptException();
    return std::nullopt;
  }

  Napi::Object binding = authObject.Get(CFG_AUTH_PROP).As<Napi::Object>();
  authRefs.emplace_back(Napi::Persistent(binding));
  Authentication *auth = Authentication::Unwrap(authRefs.back().Value());

  if (auth == nullptr || auth->GetCAuthentication() == nullptr) {
    Napi::Error::New(env, "Authentication must be a Pulsar authentication object")
        .ThrowAsJavaScriptException();
    return std::nullopt;
  }

  return auth->GetCAuthentication()->auth;
}

static std::optional<pulsar::ServiceInfo> BuildServiceInfo(const Napi::Value &value,
                                                           const std::string &fieldName,
                                                           std::vector<Napi::ObjectReference> &authRefs,
                                                           const pulsar::AuthenticationPtr &defaultAuth,
                                                           const std::optional<std::string> &defaultTls) {
  Napi::Env env = value.Env();

  if (value.IsString()) {
    std::string serviceUrl = value.ToString().Utf8Value();
    if (serviceUrl.empty()) {
      Napi::Error::New(env, fieldName + " service URL must be a non-empty string")
          .ThrowAsJavaScriptException();
      return std::nullopt;
    }
    return pulsar::ServiceInfo(serviceUrl, defaultAuth, defaultTls);
  }

  if (!value.IsObject()) {
    Napi::Error::New(env, fieldName + " must be a service URL string or service info object")
        .ThrowAsJavaScriptException();
    return std::nullopt;
  }

  Napi::Object serviceInfo = value.As<Napi::Object>();
  if (!serviceInfo.Has(CFG_SERVICE_URL) || !serviceInfo.Get(CFG_SERVICE_URL).IsString() ||
      serviceInfo.Get(CFG_SERVICE_URL).ToString().Utf8Value().empty()) {
    Napi::Error::New(env, fieldName + ".serviceUrl is required and must be a non-empty string")
        .ThrowAsJavaScriptException();
    return std::nullopt;
  }

  std::string serviceUrl = serviceInfo.Get(CFG_SERVICE_URL).ToString().Utf8Value();
  pulsar::AuthenticationPtr authentication = defaultAuth;
  std::optional<std::string> tlsTrustCertsFilePath = defaultTls;

  if (serviceInfo.Has(CFG_AUTH) && IsPresent(serviceInfo.Get(CFG_AUTH))) {
    if (!serviceInfo.Get(CFG_AUTH).IsObject()) {
      Napi::Error::New(env, fieldName + ".authentication must be a Pulsar authentication object")
          .ThrowAsJavaScriptException();
      return std::nullopt;
    }

    auto auth = BuildAuthenticationPtr(serviceInfo.Get(CFG_AUTH).As<Napi::Object>(), authRefs);
    if (!auth.has_value()) {
      return std::nullopt;
    }
    authentication = auth.value();
  }

  if (serviceInfo.Has(CFG_TLS_TRUST_CERT) && IsPresent(serviceInfo.Get(CFG_TLS_TRUST_CERT))) {
    if (!serviceInfo.Get(CFG_TLS_TRUST_CERT).IsString()) {
      Napi::Error::New(env, fieldName + ".tlsTrustCertsFilePath must be a string")
          .ThrowAsJavaScriptException();
      return std::nullopt;
    }
    tlsTrustCertsFilePath = serviceInfo.Get(CFG_TLS_TRUST_CERT).ToString().Utf8Value();
  }

  return pulsar::ServiceInfo(serviceUrl, authentication, tlsTrustCertsFilePath);
}

static bool SetPositiveUint32(const Napi::Object &config, const std::string &fieldName, uint32_t &target) {
  if (!config.Has(fieldName) || !IsPresent(config.Get(fieldName))) {
    return true;
  }

  Napi::Env env = config.Env();
  if (!config.Get(fieldName).IsNumber()) {
    Napi::Error::New(env, "serviceUrlProvider." + fieldName + " must be a positive number")
        .ThrowAsJavaScriptException();
    return false;
  }

  int64_t value = config.Get(fieldName).ToNumber().Int64Value();
  if (value <= 0 || value > UINT32_MAX) {
    Napi::Error::New(env, "serviceUrlProvider." + fieldName + " must be a positive number")
        .ThrowAsJavaScriptException();
    return false;
  }

  target = static_cast<uint32_t>(value);
  return true;
}

static std::unique_ptr<pulsar::ServiceInfoProvider> BuildServiceInfoProvider(
    const Napi::Object &clientConfig, std::vector<Napi::ObjectReference> &authRefs,
    const pulsar::AuthenticationPtr &defaultAuth, const std::optional<std::string> &defaultTls) {
  Napi::Value providerValue = clientConfig.Get(CFG_SERVICE_URL_PROVIDER);
  Napi::Env env = clientConfig.Env();

  if (!providerValue.IsObject()) {
    Napi::Error::New(env, "serviceUrlProvider must be an object").ThrowAsJavaScriptException();
    return nullptr;
  }

  Napi::Object providerConfig = providerValue.As<Napi::Object>();
  if (!providerConfig.Has(CFG_PRIMARY) || !IsPresent(providerConfig.Get(CFG_PRIMARY))) {
    Napi::Error::New(env, "serviceUrlProvider.primary is required").ThrowAsJavaScriptException();
    return nullptr;
  }

  auto primary = BuildServiceInfo(providerConfig.Get(CFG_PRIMARY), "serviceUrlProvider.primary", authRefs,
                                  defaultAuth, defaultTls);
  if (!primary.has_value()) {
    return nullptr;
  }

  if (!providerConfig.Has(CFG_SECONDARY) || !providerConfig.Get(CFG_SECONDARY).IsArray()) {
    Napi::Error::New(env, "serviceUrlProvider.secondary is required and must be an array")
        .ThrowAsJavaScriptException();
    return nullptr;
  }

  Napi::Array secondaryConfig = providerConfig.Get(CFG_SECONDARY).As<Napi::Array>();
  if (secondaryConfig.Length() == 0) {
    Napi::Error::New(env, "serviceUrlProvider.secondary must contain at least one service")
        .ThrowAsJavaScriptException();
    return nullptr;
  }

  std::vector<pulsar::ServiceInfo> secondary;
  secondary.reserve(secondaryConfig.Length());
  for (uint32_t i = 0; i < secondaryConfig.Length(); i++) {
    auto serviceInfo =
        BuildServiceInfo(secondaryConfig.Get(i), "serviceUrlProvider.secondary[" + std::to_string(i) + "]",
                         authRefs, defaultAuth, defaultTls);
    if (!serviceInfo.has_value()) {
      return nullptr;
    }
    secondary.emplace_back(std::move(serviceInfo.value()));
  }

  pulsar::AutoClusterFailover::Config autoClusterFailoverConfig(std::move(primary.value()),
                                                                std::move(secondary));

  uint32_t checkIntervalMs = static_cast<uint32_t>(autoClusterFailoverConfig.checkInterval.count());
  if (!SetPositiveUint32(providerConfig, CFG_CHECK_INTERVAL_MS, checkIntervalMs)) {
    return nullptr;
  }
  autoClusterFailoverConfig.checkInterval = std::chrono::milliseconds(checkIntervalMs);

  if (!SetPositiveUint32(providerConfig, CFG_FAILOVER_THRESHOLD,
                         autoClusterFailoverConfig.failoverThreshold)) {
    return nullptr;
  }

  if (!SetPositiveUint32(providerConfig, CFG_SWITCH_BACK_THRESHOLD,
                         autoClusterFailoverConfig.switchBackThreshold)) {
    return nullptr;
  }

  return std::unique_ptr<pulsar::ServiceInfoProvider>(
      new pulsar::AutoClusterFailover(std::move(autoClusterFailoverConfig)));
}

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

  bool hasServiceUrlProvider =
      clientConfig.Has(CFG_SERVICE_URL_PROVIDER) && IsPresent(clientConfig.Get(CFG_SERVICE_URL_PROVIDER));
  bool hasServiceUrl = clientConfig.Has(CFG_SERVICE_URL) && IsPresent(clientConfig.Get(CFG_SERVICE_URL));
  if (hasServiceUrlProvider && hasServiceUrl) {
    Napi::Error::New(env, "Only one of serviceUrl or serviceUrlProvider can be configured")
        .ThrowAsJavaScriptException();
    return;
  }

  if (!hasServiceUrlProvider && (!hasServiceUrl || !clientConfig.Get(CFG_SERVICE_URL).IsString() ||
                                 clientConfig.Get(CFG_SERVICE_URL).ToString().Utf8Value().empty())) {
    Napi::Error::New(env,
                     "Service URL is required and must be specified as a string unless serviceUrlProvider "
                     "is configured")
        .ThrowAsJavaScriptException();
    return;
  }

  this->cClientConfig = std::shared_ptr<pulsar_client_configuration_t>(pulsar_client_configuration_create(),
                                                                       pulsar_client_configuration_free);
  pulsar::AuthenticationPtr defaultAuthentication = pulsar::AuthFactory::Disabled();
  std::optional<std::string> defaultTlsTrustCertsFilePath = std::nullopt;

  // The logger can only be set once per process, so we will take control of it
  if (clientConfig.Has(CFG_LOG_LEVEL) && clientConfig.Get(CFG_LOG_LEVEL).IsNumber()) {
    int32_t logLevelInt = clientConfig.Get(CFG_LOG_LEVEL).ToNumber().Int32Value();
    this->logLevel = static_cast<pulsar_logger_level_t>(logLevelInt);
  }
  pulsar_logger_t logger;
  logger.ctx = &this->logLevel;
  logger.is_enabled = [](pulsar_logger_level_t level, void *ctx) {
    auto *logLevel = static_cast<pulsar_logger_level_t *>(ctx);
    return level >= *logLevel;
  };
  logger.log = &LogMessage;
  pulsar_client_configuration_set_logger_t(cClientConfig.get(), logger);

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
      this->authRefs_.emplace_back(Napi::Persistent(obj.Get(CFG_AUTH_PROP).As<Napi::Object>()));
      Authentication *auth = Authentication::Unwrap(this->authRefs_.back().Value());
      pulsar_client_configuration_set_auth(cClientConfig.get(), auth->GetCAuthentication());
      defaultAuthentication = auth->GetCAuthentication()->auth;
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

  if (clientConfig.Has(CFG_CONNECTION_TIMEOUT) && clientConfig.Get(CFG_CONNECTION_TIMEOUT).IsNumber()) {
    int32_t connectionTimeoutMs = clientConfig.Get(CFG_CONNECTION_TIMEOUT).ToNumber().Int32Value();
    if (connectionTimeoutMs > 0) {
      cClientConfig.get()->conf.setConnectionTimeout(connectionTimeoutMs);
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

  if (clientConfig.Has(CFG_TLS_TRUST_CERT) && clientConfig.Get(CFG_TLS_TRUST_CERT).IsString()) {
    Napi::String tlsTrustCertsFilePath = clientConfig.Get(CFG_TLS_TRUST_CERT).ToString();
    pulsar_client_configuration_set_tls_trust_certs_file_path(cClientConfig.get(),
                                                              tlsTrustCertsFilePath.Utf8Value().c_str());
    defaultTlsTrustCertsFilePath = tlsTrustCertsFilePath.Utf8Value();
  }

  if (clientConfig.Has(CFG_TLS_CERT_FILE) && clientConfig.Get(CFG_TLS_CERT_FILE).IsString()) {
    Napi::String tlsCertFilePath = clientConfig.Get(CFG_TLS_CERT_FILE).ToString();
    pulsar_client_configuration_set_tls_certificate_file_path(cClientConfig.get(),
                                                              tlsCertFilePath.Utf8Value().c_str());
  }

  if (clientConfig.Has(CFG_TLS_PRIVATE_KEY_FILE) && clientConfig.Get(CFG_TLS_PRIVATE_KEY_FILE).IsString()) {
    Napi::String tlsPrivateKeyFilePath = clientConfig.Get(CFG_TLS_PRIVATE_KEY_FILE).ToString();
    pulsar_client_configuration_set_tls_private_key_file_path(cClientConfig.get(),
                                                              tlsPrivateKeyFilePath.Utf8Value().c_str());
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

  if (clientConfig.Has(CFG_LISTENER_NAME) && clientConfig.Get(CFG_LISTENER_NAME).IsString()) {
    Napi::String listenerName = clientConfig.Get(CFG_LISTENER_NAME).ToString();
    pulsar_client_configuration_set_listener_name(cClientConfig.get(), listenerName.Utf8Value().c_str());
  }

  try {
    if (hasServiceUrlProvider) {
      std::unique_ptr<pulsar::ServiceInfoProvider> serviceInfoProvider = BuildServiceInfoProvider(
          clientConfig, this->authRefs_, defaultAuthentication, defaultTlsTrustCertsFilePath);
      if (serviceInfoProvider == nullptr) {
        return;
      }

      pulsar_client_t *rawClient = new pulsar_client_t;
      rawClient->client.reset(new pulsar::Client(
          pulsar::Client::create(std::move(serviceInfoProvider), cClientConfig.get()->conf)));
      this->cClient =
          std::shared_ptr<pulsar_client_t>(rawClient, [](pulsar_client_t *client) { delete client; });
    } else {
      Napi::String serviceUrl = clientConfig.Get(CFG_SERVICE_URL).ToString();
      this->cClient = std::shared_ptr<pulsar_client_t>(
          pulsar_client_create(serviceUrl.Utf8Value().c_str(), cClientConfig.get()), pulsar_client_free);
    }
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
