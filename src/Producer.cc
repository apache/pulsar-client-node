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
#include <pulsar/c/result.h>
#include <memory>
Napi::FunctionReference Producer::constructor;

void Producer::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env, "Producer",
                  {InstanceMethod("send", &Producer::Send), InstanceMethod("flush", &Producer::Flush),
                   InstanceMethod("close", &Producer::Close)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

void Producer::SetCProducer(pulsar_producer_t *cProducer) { this->cProducer = cProducer; }

class ProducerNewInstanceWorker : public Napi::AsyncWorker {
 public:
  ProducerNewInstanceWorker(const Napi::Promise::Deferred &deferred, pulsar_client_t *cClient,
                            ProducerConfig *producerConfig)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cClient(cClient),
        producerConfig(producerConfig) {}
  ~ProducerNewInstanceWorker() {}
  void Execute() {
    const std::string &topic = this->producerConfig->GetTopic();
    if (topic.empty()) {
      SetError(std::string("Topic is required and must be specified as a string when creating producer"));
      return;
    }

    pulsar_result result = pulsar_client_create_producer(
        this->cClient, topic.c_str(), this->producerConfig->GetCProducerConfig(), &(this->cProducer));
    delete this->producerConfig;
    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to create producer: ") + pulsar_result_str(result));
      return;
    }
  }
  void OnOK() {
    Napi::Object obj = Producer::constructor.New({});
    Producer *producer = Producer::Unwrap(obj);
    producer->SetCProducer(this->cProducer);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_client_t *cClient;
  ProducerConfig *producerConfig;
  pulsar_producer_t *cProducer;
};

Napi::Value Producer::NewInstance(const Napi::CallbackInfo &info, pulsar_client_t *cClient) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();
  ProducerConfig *producerConfig = new ProducerConfig(config);
  ProducerNewInstanceWorker *wk = new ProducerNewInstanceWorker(deferred, cClient, producerConfig);
  wk->Queue();
  return deferred.Promise();
}

Producer::Producer(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Producer>(info) {}

class ProducerSendWorker : public Napi::AsyncWorker {
 public:
  ProducerSendWorker(const Napi::Promise::Deferred &deferred, pulsar_producer_t *cProducer,
                     pulsar_message_t *cMessage)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cProducer(cProducer),
        cMessage(cMessage) {}
  ~ProducerSendWorker() { pulsar_message_free(this->cMessage); }
  void Execute() {
    pulsar_result result = pulsar_producer_send(this->cProducer, this->cMessage);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }
  void OnOK() { this->deferred.Resolve(Env().Null()); }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to send message: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_producer_t *cProducer;
  pulsar_message_t *cMessage;
};

Napi::Value Producer::Send(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  pulsar_message_t *cMessage = Message::BuildMessage(info[0].As<Napi::Object>());
  ProducerSendWorker *wk = new ProducerSendWorker(deferred, this->cProducer, cMessage);
  wk->Queue();
  return deferred.Promise();
}

class ProducerFlushWorker : public Napi::AsyncWorker {
 public:
  ProducerFlushWorker(const Napi::Promise::Deferred &deferred, pulsar_producer_t *cProducer)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cProducer(cProducer) {}

  ~ProducerFlushWorker() {}

  void Execute() {
    pulsar_result result = pulsar_producer_flush(this->cProducer);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }

  void OnOK() { this->deferred.Resolve(Env().Null()); }

  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to flush producer: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_producer_t *cProducer;
};

Napi::Value Producer::Flush(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ProducerFlushWorker *wk = new ProducerFlushWorker(deferred, this->cProducer);
  wk->Queue();
  return deferred.Promise();
}

class ProducerCloseWorker : public Napi::AsyncWorker {
 public:
  ProducerCloseWorker(const Napi::Promise::Deferred &deferred, pulsar_producer_t *cProducer)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cProducer(cProducer) {}
  ~ProducerCloseWorker() {}
  void Execute() {
    pulsar_result result = pulsar_producer_close(this->cProducer);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }
  void OnOK() { this->deferred.Resolve(Env().Null()); }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to close producer: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_producer_t *cProducer;
};

Napi::Value Producer::Close(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ProducerCloseWorker *wk = new ProducerCloseWorker(deferred, this->cProducer);
  wk->Queue();
  return deferred.Promise();
}

Producer::~Producer() { pulsar_producer_free(this->cProducer); }
