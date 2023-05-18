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

#include "Message.h"
#include "Reader.h"
#include "ReaderConfig.h"
#include "MessageId.h"
#include "ThreadSafeDeferred.h"
#include <pulsar/c/result.h>
#include <pulsar/c/reader.h>
#include <atomic>
#include <thread>
#include <future>

Napi::FunctionReference Reader::constructor;

void Reader::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Reader",
                                    {
                                        InstanceMethod("readNext", &Reader::ReadNext),
                                        InstanceMethod("hasNext", &Reader::HasNext),
                                        InstanceMethod("isConnected", &Reader::IsConnected),
                                        InstanceMethod("seek", &Reader::Seek),
                                        InstanceMethod("seekTimestamp", &Reader::SeekTimestamp),
                                        InstanceMethod("close", &Reader::Close),
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

struct ReaderListenerProxyData {
  std::shared_ptr<pulsar_message_t> cMessage;
  Reader *reader;
  std::function<void(void)> callback;

  ReaderListenerProxyData(std::shared_ptr<pulsar_message_t> cMessage, Reader *reader,
                          std::function<void(void)> callback)
      : cMessage(cMessage), reader(reader), callback(callback) {}
};

void ReaderListenerProxy(Napi::Env env, Napi::Function jsCallback, ReaderListenerProxyData *data) {
  Napi::Object msg = Message::NewInstance({}, data->cMessage);
  Reader *reader = data->reader;

  Napi::Value ret = jsCallback.Call({msg, reader->Value()});
  if (ret.IsPromise()) {
    Napi::Promise promise = ret.As<Napi::Promise>();
    Napi::Value thenValue = promise.Get("then");
    if (thenValue.IsFunction()) {
      Napi::Function then = thenValue.As<Napi::Function>();
      Napi::Function callback =
          Napi::Function::New(env, [data](const Napi::CallbackInfo &info) { data->callback(); });
      then.Call(promise, {callback});
      return;
    }
  }
  data->callback();
}

void ReaderListener(pulsar_reader_t *rawReader, pulsar_message_t *rawMessage, void *ctx) {
  std::shared_ptr<pulsar_message_t> cMessage(rawMessage, pulsar_message_free);
  ReaderListenerCallback *readerListenerCallback = (ReaderListenerCallback *)ctx;
  Reader *reader = (Reader *)readerListenerCallback->reader;
  if (readerListenerCallback->callback.Acquire() != napi_ok) {
    return;
  }

  std::promise<void> promise;
  std::future<void> future = promise.get_future();
  std::unique_ptr<ReaderListenerProxyData> dataPtr(
      new ReaderListenerProxyData(cMessage, reader, [&promise]() { promise.set_value(); }));
  readerListenerCallback->callback.BlockingCall(dataPtr.get(), ReaderListenerProxy);
  readerListenerCallback->callback.Release();

  future.wait();
}

void Reader::SetCReader(std::shared_ptr<pulsar_reader_t> cReader) { this->cReader = cReader; }
void Reader::SetListenerCallback(ReaderListenerCallback *listener) {
  if (this->listener != nullptr) {
    // It is only safe to set the listener once for the lifecycle of the Reader
    return;
  }

  if (listener != nullptr) {
    listener->reader = this;
    // If a reader listener is set, the Reader instance is kept alive even if it goes out of scope in JS code.
    this->Ref();
    this->listener = listener;
  }
}

Reader::Reader(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Reader>(info), listener(nullptr) {}

struct ReaderNewInstanceContext {
  ReaderNewInstanceContext(std::shared_ptr<ThreadSafeDeferred> deferred,
                           std::shared_ptr<pulsar_client_t> cClient,
                           std::shared_ptr<ReaderConfig> readerConfig)
      : deferred(deferred), cClient(cClient), readerConfig(readerConfig){};
  std::shared_ptr<ThreadSafeDeferred> deferred;
  std::shared_ptr<pulsar_client_t> cClient;
  std::shared_ptr<ReaderConfig> readerConfig;

  static void createReaderCallback(pulsar_result result, pulsar_reader_t *rawReader, void *ctx) {
    auto instanceContext = static_cast<ReaderNewInstanceContext *>(ctx);
    auto deferred = instanceContext->deferred;
    auto cClient = instanceContext->cClient;
    auto readerConfig = instanceContext->readerConfig;
    delete instanceContext;

    if (result != pulsar_result_Ok) {
      return deferred->Reject(std::string("Failed to create reader: ") + pulsar_result_str(result));
    }

    auto cReader = std::shared_ptr<pulsar_reader_t>(rawReader, pulsar_reader_free);

    deferred->Resolve([cReader, readerConfig](const Napi::Env env) {
      Napi::Object obj = Reader::constructor.New({});
      Reader *reader = Reader::Unwrap(obj);
      reader->SetCReader(cReader);
      reader->SetListenerCallback(readerConfig->GetListenerCallback());
      return obj;
    });
  }
};

Napi::Value Reader::NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient) {
  auto deferred = ThreadSafeDeferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();

  auto readerConfig = std::make_shared<ReaderConfig>(config, &ReaderListener);

  const std::string &topic = readerConfig->GetTopic();
  if (topic.empty()) {
    deferred->Reject(std::string("Topic is required and must be specified as a string when creating reader"));
    return deferred->Promise();
  }
  if (readerConfig->GetCStartMessageId().get() == nullptr) {
    deferred->Reject(std::string(
        "StartMessageId is required and must be specified as a MessageId object when creating reader"));
    return deferred->Promise();
  }

  auto ctx = new ReaderNewInstanceContext(deferred, cClient, readerConfig);

  pulsar_client_create_reader_async(cClient.get(), topic.c_str(), readerConfig->GetCStartMessageId().get(),
                                    readerConfig->GetCReaderConfig().get(),
                                    &ReaderNewInstanceContext::createReaderCallback, ctx);

  return deferred->Promise();
}

// We still need a read worker because the c api is missing the equivalent async definition
class ReaderReadNextWorker : public Napi::AsyncWorker {
 public:
  ReaderReadNextWorker(const Napi::Promise::Deferred &deferred, std::shared_ptr<pulsar_reader_t> cReader,
                       int64_t timeout = -1)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cReader(cReader),
        timeout(timeout) {}
  ~ReaderReadNextWorker() {}
  void Execute() {
    pulsar_result result;
    pulsar_message_t *rawMessage;
    if (timeout > 0) {
      result = pulsar_reader_read_next_with_timeout(this->cReader.get(), &rawMessage, timeout);
    } else {
      result = pulsar_reader_read_next(this->cReader.get(), &rawMessage);
    }
    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to receive message: ") + pulsar_result_str(result));
    }
    this->cMessage = std::shared_ptr<pulsar_message_t>(rawMessage, pulsar_message_free);
  }
  void OnOK() {
    Napi::Object obj = Message::NewInstance({}, this->cMessage);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  std::shared_ptr<pulsar_reader_t> cReader;
  std::shared_ptr<pulsar_message_t> cMessage;
  int64_t timeout;
};

Napi::Value Reader::ReadNext(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  if (info[0].IsUndefined()) {
    ReaderReadNextWorker *wk = new ReaderReadNextWorker(deferred, this->cReader);
    wk->Queue();
  } else {
    Napi::Number timeout = info[0].As<Napi::Object>().ToNumber();
    ReaderReadNextWorker *wk = new ReaderReadNextWorker(deferred, this->cReader, timeout.Int64Value());
    wk->Queue();
  }
  return deferred.Promise();
}

Napi::Value Reader::HasNext(const Napi::CallbackInfo &info) {
  int value = 0;
  pulsar_result result = pulsar_reader_has_message_available(this->cReader.get(), &value);
  if (result != pulsar_result_Ok) {
    Napi::Error::New(
        info.Env(), "Failed to check if next message is available: " + std::string(pulsar_result_str(result)))
        .ThrowAsJavaScriptException();
    return Napi::Boolean::New(info.Env(), false);
  } else if (value == 1) {
    return Napi::Boolean::New(info.Env(), true);
  } else {
    return Napi::Boolean::New(info.Env(), false);
  }
}

Napi::Value Reader::IsConnected(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, pulsar_reader_is_connected(this->cReader.get()));
}

Napi::Value Reader::Seek(const Napi::CallbackInfo &info) {
  auto obj = info[0].As<Napi::Object>();
  auto *msgId = MessageId::Unwrap(obj);
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_reader_seek_async(
      this->cReader.get(), msgId->GetCMessageId().get(),
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

Napi::Value Reader::SeekTimestamp(const Napi::CallbackInfo &info) {
  Napi::Number timestamp = info[0].As<Napi::Object>().ToNumber();
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);

  pulsar_reader_seek_by_timestamp_async(
      this->cReader.get(), timestamp.Int64Value(),
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

Napi::Value Reader::Close(const Napi::CallbackInfo &info) {
  auto deferred = ThreadSafeDeferred::New(Env());
  auto ctx = new ExtDeferredContext(deferred);
  this->Cleanup();

  pulsar_reader_close_async(
      this->cReader.get(),
      [](pulsar_result result, void *ctx) {
        auto deferredContext = static_cast<ExtDeferredContext *>(ctx);
        auto deferred = deferredContext->deferred;
        delete deferredContext;

        if (result != pulsar_result_Ok) {
          deferred->Reject(std::string("Failed to close reader: ") + pulsar_result_str(result));
        } else {
          deferred->Resolve(THREADSAFE_DEFERRED_RESOLVER(env.Null()));
        }
      },
      ctx);

  return deferred->Promise();
}

void Reader::Cleanup() {
  if (this->listener != nullptr) {
    this->listener->callback.Release();
    this->Unref();
    this->listener = nullptr;
  }
}

Reader::~Reader() { this->Cleanup(); }
