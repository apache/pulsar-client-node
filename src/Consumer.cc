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
#include "ThreadSafeDeferred.h"
#include "LogUtils.h"
#include <pulsar/c/result.h>
#include <atomic>
#include <thread>
#include <future>
#include <sstream>

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
                      InstanceMethod("seek", &Consumer::Seek),
                      InstanceMethod("seekTimestamp", &Consumer::SeekTimestamp),
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
  std::function<void(void)> callback;

  MessageListenerProxyData(std::shared_ptr<pulsar_message_t> cMessage, Consumer *consumer,
                           std::function<void(void)> callback)
      : cMessage(cMessage), consumer(consumer), callback(callback) {}
};

inline void logMessageListenerError(Consumer *consumer, const char *err) {
  std::ostringstream ss;
  ss << "[" << consumer->GetTopic() << "][" << consumer->GetSubscriptionName()
     << "] Message listener error in processing message: " << err;
  LOG_ERROR(ss.str().c_str());
}

void MessageListenerProxy(Napi::Env env, Napi::Function jsCallback, MessageListenerProxyData *data) {
  Napi::Object msg = Message::NewInstance({}, data->cMessage);
  Consumer *consumer = data->consumer;

  // `consumer` might be null in certain cases, segmentation fault might happend without this null check. We
  // need to handle this rare case in future.
  if (consumer) {
    Napi::Value ret;
    try {
      ret = jsCallback.Call({msg, consumer->Value()});
    } catch (std::exception &exception) {
      logMessageListenerError(consumer, exception.what());
    }

    if (ret.IsPromise()) {
      Napi::Promise promise = ret.As<Napi::Promise>();
      Napi::Function catchFunc = promise.Get("catch").As<Napi::Function>();

      ret = catchFunc.Call(promise, {Napi::Function::New(env, [consumer](const Napi::CallbackInfo &info) {
                             Napi::Error error = info[0].As<Napi::Error>();
                             logMessageListenerError(consumer, error.what());
                           })});

      promise = ret.As<Napi::Promise>();
      Napi::Function finallyFunc = promise.Get("finally").As<Napi::Function>();

      finallyFunc.Call(
          promise, {Napi::Function::New(env, [data](const Napi::CallbackInfo &info) { data->callback(); })});
      return;
    }
  }
  data->callback();
}

void MessageListener(pulsar_consumer_t *rawConsumer, pulsar_message_t *rawMessage, void *ctx) {
  std::shared_ptr<pulsar_message_t> cMessage(rawMessage, pulsar_message_free);
  MessageListenerCallback *listenerCallback = (MessageListenerCallback *)ctx;

  Consumer *consumer = (Consumer *)listenerCallback->consumer;

  if (listenerCallback->callback.Acquire() != napi_ok) {
    return;
  }

  std::promise<void> promise;
  std::future<void> future = promise.get_future();
  std::unique_ptr<MessageListenerProxyData> dataPtr(
      new MessageListenerProxyData(cMessage, consumer, [&promise]() { promise.set_value(); }));
  listenerCallback->callback.BlockingCall(dataPtr.get(), MessageListenerProxy);
  listenerCallback->callback.Release();

  future.wait();
}

void Consumer::SetCConsumer(std::shared_ptr<pulsar_consumer_t> cConsumer) { this->cConsumer = cConsumer; }
void Consumer::SetListenerCallback(MessageListenerCallback *listener) {
  if (this->listener != nullptr) {
    // It is only safe to set the listener once for the lifecycle of the Consumer
    return;
  }

  if (listener != nullptr) {
    listener->consumer = this;
    // If a consumer listener is set, the Consumer instance is kept alive even if it goes out of scope in JS
    // code.
    this->Ref();
    this->listener = listener;
  }
}

Consumer::Consumer(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Consumer>(info), listener(nullptr) {}

struct ConsumerNewInstanceContext {
  ConsumerNewInstanceContext(std::shared_ptr<ThreadSafeDeferred> deferred,
                             std::shared_ptr<pulsar_client_t> cClient,
                             std::shared_ptr<ConsumerConfig> consumerConfig)
      : deferred(deferred), cClient(cClient), consumerConfig(consumerConfig){};
  std::shared_ptr<ThreadSafeDeferred> deferred;
  std::shared_ptr<pulsar_client_t> cClient;
  std::shared_ptr<ConsumerConfig> consumerConfig;

  static void subscribeCallback(pulsar_result result, pulsar_consumer_t *rawConsumer, void *ctx) {
    auto instanceContext = static_cast<ConsumerNewInstanceContext *>(ctx);
    auto deferred = instanceContext->deferred;
    auto cClient = instanceContext->cClient;
    auto consumerConfig = instanceContext->consumerConfig;
    delete instanceContext;

    if (result != pulsar_result_Ok) {
      return deferred->Reject(std::string("Failed to create consumer: ") + pulsar_result_str(result));
    }

    auto cConsumer = std::shared_ptr<pulsar_consumer_t>(rawConsumer, pulsar_consumer_free);
    auto listener = consumerConfig->GetListenerCallback();

    if (listener) {
      // pause, will resume in OnOK, to prevent MessageListener get a nullptr of consumer
      pulsar_consumer_pause_message_listener(cConsumer.get());
    }

    deferred->Resolve([cConsumer, consumerConfig, listener](const Napi::Env env) {
      Napi::Object obj = Consumer::constructor.New({});
      Consumer *consumer = Consumer::Unwrap(obj);

      consumer->SetCConsumer(cConsumer);
      consumer->SetListenerCallback(listener);

      if (listener) {
        // resume to enable MessageListener function callback
        resume_message_listener(cConsumer.get());
      }

      return obj;
    });
  }
};

Napi::Value Consumer::NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient) {
  auto deferred = ThreadSafeDeferred::New(info.Env());
  auto config = info[0].As<Napi::Object>();
  std::shared_ptr<ConsumerConfig> consumerConfig = std::make_shared<ConsumerConfig>(config, &MessageListener);

  const std::string &topic = consumerConfig->GetTopic();
  const std::vector<std::string> &topics = consumerConfig->GetTopics();
  const std::string &topicsPattern = consumerConfig->GetTopicsPattern();
  if (topic.empty() && topics.size() == 0 && topicsPattern.empty()) {
    deferred->Reject(
        std::string("Topic, topics or topicsPattern is required and must be specified as a string when "
                    "creating consumer"));
    return deferred->Promise();
  }
  const std::string &subscription = consumerConfig->GetSubscription();
  if (subscription.empty()) {
    deferred->Reject(
        std::string("Subscription is required and must be specified as a string when creating consumer"));
    return deferred->Promise();
  }
  int32_t ackTimeoutMs = consumerConfig->GetAckTimeoutMs();
  if (ackTimeoutMs != 0 && ackTimeoutMs < MIN_ACK_TIMEOUT_MILLIS) {
    std::string msg("Ack timeout should be 0 or greater than or equal to " +
                    std::to_string(MIN_ACK_TIMEOUT_MILLIS));
    deferred->Reject(msg);
    return deferred->Promise();
  }
  int32_t nAckRedeliverTimeoutMs = consumerConfig->GetNAckRedeliverTimeoutMs();
  if (nAckRedeliverTimeoutMs < 0) {
    std::string msg("NAck timeout should be greater than or equal to zero");
    deferred->Reject(msg);
    return deferred->Promise();
  }

  auto ctx = new ConsumerNewInstanceContext(deferred, cClient, consumerConfig);

  if (!topicsPattern.empty()) {
    pulsar_client_subscribe_pattern_async(cClient.get(), topicsPattern.c_str(), subscription.c_str(),
                                          consumerConfig->GetCConsumerConfig().get(),
                                          &ConsumerNewInstanceContext::subscribeCallback, ctx);
  } else if (topics.size() > 0) {
    const char **cTopics = new const char *[topics.size()];
    for (size_t i = 0; i < topics.size(); i++) {
      cTopics[i] = topics[i].c_str();
    }
    pulsar_client_subscribe_multi_topics_async(cClient.get(), cTopics, topics.size(), subscription.c_str(),
                                               consumerConfig->GetCConsumerConfig().get(),
                                               &ConsumerNewInstanceContext::subscribeCallback, ctx);
    delete[] cTopics;
  } else {
    pulsar_client_subscribe_async(cClient.get(), topic.c_str(), subscription.c_str(),
                                  consumerConfig->GetCConsumerConfig().get(),
                                  &ConsumerNewInstanceContext::subscribeCallback, ctx);
  }

  return deferred->Promise();
}

std::string Consumer::GetTopic() { return {pulsar_consumer_get_topic(this->cConsumer.get())}; }

std::string Consumer::GetSubscriptionName() {
  return {pulsar_consumer_get_subscription_name(this->cConsumer.get())};
}

// We still need a receive worker because the c api is missing the equivalent async definition
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
      SetError(std::string("Failed to receive message: ") + pulsar_result_str(result));
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
  if (info[0].IsUndefined()) {
    auto deferred = ThreadSafeDeferred::New(Env());
    auto ctx = new ExtDeferredContext(deferred);
    pulsar_consumer_receive_async(
        this->cConsumer.get(),
        [](pulsar_result result, pulsar_message_t *rawMessage, void *ctx) {
          auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
          auto deferred = deferredContext->deferred;
          delete deferredContext;

          if (result != pulsar_result_Ok) {
            deferred->Reject(std::string("Failed to receive message: ") + pulsar_result_str(result));
          } else {
            deferred->Resolve([rawMessage](const Napi::Env env) {
              Napi::Object obj = Message::NewInstance(
                  {}, std::shared_ptr<pulsar_message_t>(rawMessage, pulsar_message_free));
              return obj;
            });
          }
        },
        ctx);
    return deferred->Promise();
  } else {
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
    Napi::Number timeout = info[0].As<Napi::Object>().ToNumber();
    ConsumerReceiveWorker *wk = new ConsumerReceiveWorker(deferred, this->cConsumer, timeout.Int64Value());
    wk->Queue();
    return deferred.Promise();
  }
}

Napi::Value Consumer::Acknowledge(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto msg = Message::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_acknowledge_async(
      this->cConsumer.get(), msg->GetCMessage().get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to acknowledge: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::AcknowledgeId(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto *msgId = MessageId::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_acknowledge_async_id(
      this->cConsumer.get(), msgId->GetCMessageId().get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to acknowledge id: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

void Consumer::NegativeAcknowledge(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  Message *msg = Message::Unwrap(obj);
  std::shared_ptr<pulsar_message_t> cMessage = msg->GetCMessage();
  pulsar_consumer_negative_acknowledge(this->cConsumer.get(), cMessage.get());
}

void Consumer::NegativeAcknowledgeId(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = MessageId::Unwrap(obj);
  std::shared_ptr<pulsar_message_id_t> cMessageId = msgId->GetCMessageId();
  pulsar_consumer_negative_acknowledge_id(this->cConsumer.get(), cMessageId.get());
}

Napi::Value Consumer::AcknowledgeCumulative(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto *msg = Message::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_acknowledge_cumulative_async(
      this->cConsumer.get(), msg->GetCMessage().get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to acknowledge cumulatively: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::AcknowledgeCumulativeId(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto *msgId = MessageId::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_acknowledge_cumulative_async_id(
      this->cConsumer.get(), msgId->GetCMessageId().get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to acknowledge cumulatively by id: ") +
                           pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::Seek(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto *msgId = MessageId::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_seek_async(
      this->cConsumer.get(), msgId->GetCMessageId().get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to seek message by id: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::SeekTimestamp(const Napi::CallbackInfo &info) {
  Napi::Number timestamp = info[0].As<Napi::Object>().ToNumber();
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_seek_by_timestamp_async(
      this->cConsumer.get(), timestamp.Int64Value(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to seek message by timestamp: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::IsConnected(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, pulsar_consumer_is_connected(this->cConsumer.get()));
}

void Consumer::Cleanup() {
  if (this->listener != nullptr) {
    pulsar_consumer_pause_message_listener(this->cConsumer.get());
    this->listener->callback.Release();
    this->listener = nullptr;
    this->Unref();
  }
}

Napi::Value Consumer::Close(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);
  this->Cleanup();

  pulsar_consumer_close_async(
      this->cConsumer.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to close consumer: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Napi::Value Consumer::Unsubscribe(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_consumer_pause_message_listener(this->cConsumer.get());
  pulsar_consumer_unsubscribe_async(
      this->cConsumer.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to unsubscribe consumer: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

Consumer::~Consumer() { this->Cleanup(); }
