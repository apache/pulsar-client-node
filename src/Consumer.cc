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

#include "Consumer.h"
#include "ConsumerConfig.h"
#include "Message.h"
#include "MessageId.h"
#include <pulsar/c/result.h>

Napi::FunctionReference Consumer::constructor;

void Consumer::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env, "Consumer",
                  {
                      InstanceMethod("receive", &Consumer::Receive),
                      InstanceMethod("acknowledge", &Consumer::Acknowledge),
                      InstanceMethod("acknowledgeId", &Consumer::AcknowledgeId),
                      InstanceMethod("acknowledgeCumulative", &Consumer::AcknowledgeCumulative),
                      InstanceMethod("acknowledgeCumulativeId", &Consumer::AcknowledgeCumulativeId),
                      InstanceMethod("close", &Consumer::Close),
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

void Consumer::SetCConsumer(pulsar_consumer_t *cConsumer) { this->cConsumer = cConsumer; }

Consumer::Consumer(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Consumer>(info) {}

class ConsumerNewInstanceWorker : public Napi::AsyncWorker {
 public:
  ConsumerNewInstanceWorker(const Napi::Promise::Deferred &deferred, pulsar_client_t *cClient,
                            ConsumerConfig *consumerConfig)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cClient(cClient),
        consumerConfig(consumerConfig) {}
  ~ConsumerNewInstanceWorker() {}
  void Execute() {
    const std::string &topic = this->consumerConfig->GetTopic();
    if (topic.empty()) {
      SetError(std::string("Topic is required and must be specified as a string when creating consumer"));
      return;
    }
    const std::string &subscription = this->consumerConfig->GetSubscription();
    if (subscription.empty()) {
      SetError(
          std::string("Subscription is required and must be specified as a string when creating consumer"));
      return;
    }
    int32_t ackTimeoutMs = this->consumerConfig->GetAckTimeoutMs();
    if (ackTimeoutMs != 0 && ackTimeoutMs < MIN_ACK_TIMEOUT_MILLIS) {
      std::string msg("Ack timeout should be 0 or greater than or equal to " +
                      std::to_string(MIN_ACK_TIMEOUT_MILLIS));
      SetError(msg);
      return;
    }

    pulsar_result result =
        pulsar_client_subscribe(this->cClient, topic.c_str(), subscription.c_str(),
                                this->consumerConfig->GetCConsumerConfig(), &(this->cConsumer));
    delete this->consumerConfig;
    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to create consumer: ") + pulsar_result_str(result));
      return;
    }
  }
  void OnOK() {
    Napi::Object obj = Consumer::constructor.New({});
    Consumer *consumer = Consumer::Unwrap(obj);
    consumer->SetCConsumer(this->cConsumer);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_client_t *cClient;
  ConsumerConfig *consumerConfig;
  pulsar_consumer_t *cConsumer;
};

Napi::Value Consumer::NewInstance(const Napi::CallbackInfo &info, pulsar_client_t *cClient) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();
  ConsumerConfig *consumerConfig = new ConsumerConfig(config);
  ConsumerNewInstanceWorker *wk = new ConsumerNewInstanceWorker(deferred, cClient, consumerConfig);
  wk->Queue();
  return deferred.Promise();
}

class ConsumerReceiveWorker : public Napi::AsyncWorker {
 public:
  ConsumerReceiveWorker(const Napi::Promise::Deferred &deferred, pulsar_consumer_t *cConsumer,
                        int64_t timeout = -1)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cConsumer(cConsumer),
        timeout(timeout) {}
  ~ConsumerReceiveWorker() {}
  void Execute() {
    pulsar_result result;
    if (timeout > 0) {
      result = pulsar_consumer_receive_with_timeout(this->cConsumer, &(this->cMessage), timeout);
    } else {
      result = pulsar_consumer_receive(this->cConsumer, &(this->cMessage));
    }

    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to received message ") + pulsar_result_str(result));
    }
  }
  void OnOK() {
    Napi::Object obj = Message::NewInstance({}, this->cMessage);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_consumer_t *cConsumer;
  pulsar_message_t *cMessage;
  int64_t timeout;
};

Napi::Value Consumer::Receive(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  if (info[0].IsUndefined()) {
    ConsumerReceiveWorker *wk = new ConsumerReceiveWorker(deferred, this->cConsumer);
    wk->Queue();
  } else {
    Napi::Number timeout = info[0].As<Napi::Object>().ToNumber();
    ConsumerReceiveWorker *wk = new ConsumerReceiveWorker(deferred, this->cConsumer, timeout.Int64Value());
    wk->Queue();
  }
  return deferred.Promise();
}

void Consumer::Acknowledge(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  pulsar_consumer_acknowledge_async(this->cConsumer, msg->GetCMessage(), NULL, NULL);
}

void Consumer::AcknowledgeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  pulsar_consumer_acknowledge_async_id(this->cConsumer, msgId->GetCMessageId(), NULL, NULL);
}

void Consumer::AcknowledgeCumulative(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  pulsar_consumer_acknowledge_cumulative_async(this->cConsumer, msg->GetCMessage(), NULL, NULL);
}

void Consumer::AcknowledgeCumulativeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  pulsar_consumer_acknowledge_cumulative_async_id(this->cConsumer, msgId->GetCMessageId(), NULL, NULL);
}

class ConsumerCloseWorker : public Napi::AsyncWorker {
 public:
  ConsumerCloseWorker(const Napi::Promise::Deferred &deferred, pulsar_consumer_t *cConsumer)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cConsumer(cConsumer) {}
  ~ConsumerCloseWorker() {}
  void Execute() {
    pulsar_result result = pulsar_consumer_close(this->cConsumer);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }
  void OnOK() { this->deferred.Resolve(Env().Null()); }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to close consumer: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_consumer_t *cConsumer;
};

Napi::Value Consumer::Close(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ConsumerCloseWorker *wk = new ConsumerCloseWorker(deferred, this->cConsumer);
  wk->Queue();
  return deferred.Promise();
}

Consumer::~Consumer() { pulsar_consumer_free(this->cConsumer); }
