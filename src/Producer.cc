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

#include "Producer.h"
#include "ProducerConfig.h"
#include "Message.h"
#include "MessageId.h"
#include "ThreadSafeDeferred.h"
#include <pulsar/c/result.h>
#include <memory>
Napi::FunctionReference Producer::constructor;

void Producer::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env, "Producer",
                  {InstanceMethod("send", &Producer::Send), InstanceMethod("flush", &Producer::Flush),
                   InstanceMethod("close", &Producer::Close),
                   InstanceMethod("getProducerName", &Producer::GetProducerName),
                   InstanceMethod("getTopic", &Producer::GetTopic),
                   InstanceMethod("isConnected", &Producer::IsConnected)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

void Producer::SetCProducer(std::shared_ptr<pulsar_producer_t> cProducer) { this->cProducer = cProducer; }

struct ProducerNewInstanceContext {
  ProducerNewInstanceContext(std::shared_ptr<ThreadSafeDeferred> deferred,
                             std::shared_ptr<pulsar_client_t> cClient,
                             std::shared_ptr<ProducerConfig> producerConfig)
      : deferred(deferred), cClient(cClient), producerConfig(producerConfig){};
  std::shared_ptr<ThreadSafeDeferred> deferred;
  std::shared_ptr<pulsar_client_t> cClient;
  std::shared_ptr<ProducerConfig> producerConfig;
};

Napi::Value Producer::NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient) {
  auto deferred = ThreadSafeDeferred::New(info.Env());
  auto config = info[0].As<Napi::Object>();
  auto producerConfig = std::make_shared<ProducerConfig>(config);

  const std::string &topic = producerConfig->GetTopic();
  if (topic.empty()) {
    deferred->Reject(
        std::string("Topic is required and must be specified as a string when creating producer"));
    return deferred->Promise();
  }

  auto ctx = new ProducerNewInstanceContext(deferred, cClient, producerConfig);

  pulsar_client_create_producer_async(
      cClient.get(), topic.c_str(), producerConfig->GetCProducerConfig().get(),
      [](pulsar_result result, pulsar_producer_t *rawProducer, void *ctx) {
        auto instanceContext = static_cast<ProducerNewInstanceContext *>(ctx);
        auto deferred = instanceContext->deferred;
        auto cClient = instanceContext->cClient;
        delete instanceContext;

        if (result != pulsar_result_Ok) {
          return deferred->Reject(std::string("Failed to create producer: ") + pulsar_result_str(result));
        }

        std::shared_ptr<pulsar_producer_t> cProducer(rawProducer, pulsar_producer_free);

        deferred->Resolve([cProducer](const Napi::Env env) {
          Napi::Object obj = Producer::constructor.New({});
          Producer *producer = Producer::Unwrap(obj);
          producer->SetCProducer(cProducer);
          return obj;
        });
      },
      ctx);

  return deferred->Promise();
}

Producer::Producer(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Producer>(info) {}

struct ProducerSendContext {
  ProducerSendContext(std::shared_ptr<ThreadSafeDeferred> deferred,
                      std::shared_ptr<pulsar_message_t> cMessage)
      : deferred(deferred), cMessage(cMessage){};
  std::shared_ptr<ThreadSafeDeferred> deferred;
  std::shared_ptr<pulsar_message_t> cMessage;
};

Napi::Value Producer::Send(const Napi::CallbackInfo &info) {
  auto cMessage = Message::BuildMessage(info[0].As<Napi::Object>());
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ProducerSendContext(deferred, cMessage);

  pulsar_producer_send_async(
      this->cProducer.get(), cMessage.get(),
      [](pulsar_result result, pulsar_message_id_t *msgId, void *ctx) {
        auto producerSendContext = static_cast<ProducerSendContext *>(ctx);
        auto deferred = producerSendContext->deferred;
        auto cMessage = producerSendContext->cMessage;
        delete producerSendContext;

        std::shared_ptr<pulsar_message_id_t> cMessageId(msgId, pulsar_message_id_free);

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to send message: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve([cMessageId](const Napi::Env env) { return MessageId::NewInstance(cMessageId); });
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Producer::Flush(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_producer_flush_async(
      this->cProducer.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to flush producer: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Producer::Close(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_producer_close_async(
      this->cProducer.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to close producer: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Producer::GetProducerName(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::String::New(env, pulsar_producer_get_producer_name(this->cProducer.get()));
}

Napi::Value Producer::GetTopic(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::String::New(env, pulsar_producer_get_topic(this->cProducer.get()));
}

Napi::Value Producer::IsConnected(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, pulsar_producer_is_connected(this->cProducer.get()));
}

Producer::~Producer() {}
