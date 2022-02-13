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
#include <atomic>
#include <thread>

Napi::FunctionReference Consumer::constructor;

void Consumer::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env, "Consumer",
                  {
                      InstanceMethod("receive", &Consumer::Receive),
                      InstanceMethod("acknowledge", &Consumer::Acknowledge),
                      InstanceMethod("acknowledgeId", &Consumer::AcknowledgeId),
                      InstanceMethod("negativeAcknowledge", &Consumer::NegativeAcknowledge),
                      InstanceMethod("negativeAcknowledgeId", &Consumer::NegativeAcknowledgeId),
                      InstanceMethod("acknowledgeCumulative", &Consumer::AcknowledgeCumulative),
                      InstanceMethod("acknowledgeCumulativeId", &Consumer::AcknowledgeCumulativeId),
                      InstanceMethod("isConnected", &Consumer::IsConnected),
                      InstanceMethod("close", &Consumer::Close),
                      InstanceMethod("unsubscribe", &Consumer::Unsubscribe),
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

struct MessageListenerProxyData {
  std::shared_ptr<pulsar_message_t> cMessage;
  Consumer *consumer;

  MessageListenerProxyData(std::shared_ptr<pulsar_message_t> cMessage, Consumer *consumer)
      : cMessage(cMessage), consumer(consumer) {}
};

void MessageListenerProxy(Napi::Env env, Napi::Function jsCallback, MessageListenerProxyData *data) {
  Napi::Object msg = Message::NewInstance({}, data->cMessage);
  Consumer *consumer = data->consumer;
  delete data;

  jsCallback.Call({msg, consumer->Value()});
}

void MessageListener(pulsar_consumer_t *rawConsumer, pulsar_message_t *rawMessage, void *ctx) {
  std::shared_ptr<pulsar_message_t> cMessage(rawMessage, pulsar_message_free);
  MessageListenerCallback *listenerCallback = (MessageListenerCallback *)ctx;

  Consumer *consumer = (Consumer *)listenerCallback->consumer;

  if (listenerCallback->callback.Acquire() != napi_ok) {
    return;
  }

  MessageListenerProxyData *dataPtr = new MessageListenerProxyData(cMessage, consumer);
  listenerCallback->callback.BlockingCall(dataPtr, MessageListenerProxy);
  listenerCallback->callback.Release();
}

void Consumer::SetCConsumer(std::shared_ptr<pulsar_consumer_t> cConsumer) { this->cConsumer = cConsumer; }
void Consumer::SetListenerCallback(MessageListenerCallback *listener) {
  if (listener) {
    // Pass consumer as argument
    listener->consumer = this;
  }

  if (this->listener == nullptr) {
    // Maintain reference to consumer, so it won't get garbage collected
    // since, when we have a listener, we don't have to maintain reference to consumer (in js code)
    this->Ref();
  }

  this->listener = listener;
}

Consumer::Consumer(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Consumer>(info), listener(nullptr) {}

class ConsumerNewInstanceWorker : public Napi::AsyncWorker {
 public:
  ConsumerNewInstanceWorker(const Napi::Promise::Deferred &deferred, std::shared_ptr<pulsar_client_t> cClient,
                            ConsumerConfig *consumerConfig)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cClient(cClient),
        consumerConfig(consumerConfig) {}
  ~ConsumerNewInstanceWorker() {}
  void Execute() {
    const std::string &topic = this->consumerConfig->GetTopic();
    const std::vector<std::string> &topics = this->consumerConfig->GetTopics();
    const std::string &topicsPattern = this->consumerConfig->GetTopicsPattern();
    if (topic.empty() && topics.size() == 0 && topicsPattern.empty()) {
      SetError(
          std::string("Topic, topics or topicsPattern is required and must be specified as a string when "
                      "creating consumer"));
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
    int32_t nAckRedeliverTimeoutMs = this->consumerConfig->GetNAckRedeliverTimeoutMs();
    if (nAckRedeliverTimeoutMs < 0) {
      std::string msg("NAck timeout should be greater than or equal to zero");
      SetError(msg);
      return;
    }

    this->done = false;
    if (!topicsPattern.empty()) {
      pulsar_client_subscribe_pattern_async(this->cClient.get(), topicsPattern.c_str(), subscription.c_str(),
                                            this->consumerConfig->GetCConsumerConfig().get(),
                                            &ConsumerNewInstanceWorker::subscribeCallback, (void *)this);
    } else if (topics.size() > 0) {
      const char **cTopics = new const char *[topics.size()];
      for (size_t i = 0; i < topics.size(); i++) {
        cTopics[i] = topics[i].c_str();
      }
      pulsar_client_subscribe_multi_topics_async(this->cClient.get(), cTopics, topics.size(), subscription.c_str(),
                                                 this->consumerConfig->GetCConsumerConfig().get(),
                                                 &ConsumerNewInstanceWorker::subscribeCallback, (void *)this);
      delete cTopics;
    } else {
      pulsar_client_subscribe_async(this->cClient.get(), topic.c_str(), subscription.c_str(),
                                    this->consumerConfig->GetCConsumerConfig().get(),
                                    &ConsumerNewInstanceWorker::subscribeCallback, (void *)this);
    }
    while (!done) {
      std::this_thread::yield();
    }
  }
  void OnOK() {
    Napi::Object obj = Consumer::constructor.New({});
    Consumer *consumer = Consumer::Unwrap(obj);

    consumer->SetCConsumer(this->cConsumer);
    consumer->SetListenerCallback(this->listener);

    if (this->listener) {
      // resume to enable MessageListener function callback
      resume_message_listener(this->cConsumer.get());
    }

    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  std::shared_ptr<pulsar_client_t> cClient;
  std::shared_ptr<pulsar_consumer_t> cConsumer;
  ConsumerConfig *consumerConfig;
  MessageListenerCallback *listener;
  std::atomic<bool> done;
  static void subscribeCallback(pulsar_result result, pulsar_consumer_t *rawConsumer, void *ctx) {
    ConsumerNewInstanceWorker *worker = (ConsumerNewInstanceWorker *)ctx;
    if (result != pulsar_result_Ok) {
      worker->SetError(std::string("Failed to create consumer: ") + pulsar_result_str(result));
    } else {
      worker->cConsumer = std::shared_ptr<pulsar_consumer_t>(rawConsumer, pulsar_consumer_free);
      worker->listener = worker->consumerConfig->GetListenerCallback();

      if (worker->listener) {
        // pause, will resume in OnOK, to prevent MessageListener get a nullptr of consumer
        pulsar_consumer_pause_message_listener(worker->cConsumer.get());
      }
    }

    delete worker->consumerConfig;
    worker->done = true;
  }
};

Napi::Value Consumer::NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();

  ConsumerConfig *consumerConfig = new ConsumerConfig(config, &MessageListener);
  ConsumerNewInstanceWorker *wk =
      new ConsumerNewInstanceWorker(deferred, cClient, consumerConfig);
  wk->Queue();
  return deferred.Promise();
}

class ConsumerReceiveWorker : public Napi::AsyncWorker {
 public:
  ConsumerReceiveWorker(const Napi::Promise::Deferred &deferred, std::shared_ptr<pulsar_consumer_t> cConsumer,
                        int64_t timeout = -1)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cConsumer(cConsumer),
        timeout(timeout) {}
  ~ConsumerReceiveWorker() {}
  void Execute() {
    pulsar_result result;
    pulsar_message_t *rawMessage;
    if (timeout > 0) {
      result = pulsar_consumer_receive_with_timeout(this->cConsumer.get(), &rawMessage, timeout);
    } else {
      result = pulsar_consumer_receive(this->cConsumer.get(), &rawMessage);
    }

    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to received message ") + pulsar_result_str(result));
    } else {
      this->cMessage = std::shared_ptr<pulsar_message_t>(rawMessage, pulsar_message_free);
    }
  }
  void OnOK() {
    Napi::Object obj = Message::NewInstance({}, this->cMessage);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  std::shared_ptr<pulsar_consumer_t> cConsumer;
  std::shared_ptr<pulsar_message_t> cMessage;
  int64_t timeout;
};

Napi::Value Consumer::Receive(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  if (info[0].IsUndefined()) {
    ConsumerReceiveWorker *wk = new ConsumerReceiveWorker(deferred, this->cConsumer);
    wk->Queue();
  } else {
    Napi::Number timeout = info[0].As<Napi::Object>().ToNumber();
    ConsumerReceiveWorker *wk =
        new ConsumerReceiveWorker(deferred, this->cConsumer, timeout.Int64Value());
    wk->Queue();
  }
  return deferred.Promise();
}

void Consumer::Acknowledge(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  std::shared_ptr<pulsar_message_t> cMessage = msg->GetCMessage();
  pulsar_consumer_acknowledge_async(this->cConsumer.get(), cMessage.get(), NULL, NULL);
}

void Consumer::AcknowledgeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  std::shared_ptr<pulsar_message_id_t> cMessageId = msgId->GetCMessageId();
  // TODO: wrapper and message id must persist for the duration of the function call
  pulsar_consumer_acknowledge_async_id(this->cConsumer.get(), cMessageId.get(), NULL, NULL);
}

void Consumer::NegativeAcknowledge(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  std::shared_ptr<pulsar_message_t> cMessage = msg->GetCMessage();
  // TODO: wrapper and message must persist for the duration of the function call
  pulsar_consumer_negative_acknowledge(this->cConsumer.get(), cMessage.get());
}

void Consumer::NegativeAcknowledgeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  std::shared_ptr<pulsar_message_id_t> cMessageId = msgId->GetCMessageId();
  // TODO: wrapper and message id must persist for the duration of the function call
  pulsar_consumer_negative_acknowledge_id(this->cConsumer.get(), cMessageId.get());
}

void Consumer::AcknowledgeCumulative(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  std::shared_ptr<pulsar_message_t> cMessage = msg->GetCMessage();
  // TODO: wrapper and message must persist for the duration of the function call
  pulsar_consumer_acknowledge_cumulative_async(this->cConsumer.get(), cMessage.get(), NULL, NULL);
}

void Consumer::AcknowledgeCumulativeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  std::shared_ptr<pulsar_message_id_t> cMessageId = msgId->GetCMessageId();
  // TODO: wrapper and message id must persist for the duration of the function call
  pulsar_consumer_acknowledge_cumulative_async_id(this->cConsumer.get(), cMessageId.get(), NULL,
                                                  NULL);
}

Napi::Value Consumer::IsConnected(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, pulsar_consumer_is_connected(this->cConsumer.get()));
}

class ConsumerCloseWorker : public Napi::AsyncWorker {
 public:
  ConsumerCloseWorker(const Napi::Promise::Deferred &deferred, std::shared_ptr<pulsar_consumer_t> cConsumer,
                      Consumer *consumer)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cConsumer(cConsumer),
        consumer(consumer) {}

  ~ConsumerCloseWorker() {}
  void Execute() {
    pulsar_consumer_pause_message_listener(this->cConsumer.get());
    pulsar_result result = pulsar_consumer_close(this->cConsumer.get());
    if (result != pulsar_result_Ok) {
      SetError(pulsar_result_str(result));
    }
  }
  void OnOK() {
    this->consumer->Cleanup();
    this->deferred.Resolve(Env().Null());
  }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to close consumer: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  std::shared_ptr<pulsar_consumer_t> cConsumer;
  Consumer *consumer;
};

class ConsumerUnsubscribeWorker : public Napi::AsyncWorker {
 public:
  ConsumerUnsubscribeWorker(const Napi::Promise::Deferred &deferred, std::shared_ptr<pulsar_consumer_t> cConsumer,
                            Consumer *consumer)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cConsumer(cConsumer),
        consumer(consumer) {}

  ~ConsumerUnsubscribeWorker() {}
  void Execute() {
    pulsar_consumer_pause_message_listener(this->cConsumer.get());
    pulsar_result result = pulsar_consumer_unsubscribe(this->cConsumer.get());
    if (result != pulsar_result_Ok) {
      SetError(pulsar_result_str(result));
    }
  }
  void OnOK() {
    this->consumer->Cleanup();
    this->deferred.Resolve(Env().Null());
  }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to unsubscribe consumer: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  std::shared_ptr<pulsar_consumer_t> cConsumer;
  Consumer *consumer;
};

void Consumer::Cleanup() {
  if (this->listener) {
    this->CleanupListener();
  }
}

void Consumer::CleanupListener() {
  pulsar_consumer_pause_message_listener(this->cConsumer.get());
  this->Unref();
  this->listener->callback.Release();
  this->listener = nullptr;
}

Napi::Value Consumer::Close(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ConsumerCloseWorker *wk = new ConsumerCloseWorker(deferred, this->cConsumer, this);
  wk->Queue();
  return deferred.Promise();
}

Napi::Value Consumer::Unsubscribe(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ConsumerUnsubscribeWorker *wk = new ConsumerUnsubscribeWorker(deferred, this->cConsumer, this);
  wk->Queue();
  return deferred.Promise();
}

Consumer::~Consumer() {
  if (this->listener) {
    this->CleanupListener();
  }
}
