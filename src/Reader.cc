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
#include <pulsar/c/result.h>
#include <pulsar/c/reader.h>
#include <atomic>
#include <thread>

Napi::FunctionReference Reader::constructor;

void Reader::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Reader",
                                    {
                                        InstanceMethod("readNext", &Reader::ReadNext),
                                        InstanceMethod("hasNext", &Reader::HasNext),
                                        InstanceMethod("isConnected", &Reader::IsConnected),
                                        InstanceMethod("close", &Reader::Close),
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

struct ReaderListenerProxyData {
  pulsar_message_t *cMessage;
  Reader *reader;

  ReaderListenerProxyData(pulsar_message_t *cMessage, Reader *reader) : cMessage(cMessage), reader(reader) {}
};

void ReaderListenerProxy(Napi::Env env, Napi::Function jsCallback, ReaderListenerProxyData *data) {
  Napi::Object msg = Message::NewInstance({}, data->cMessage);
  Reader *reader = data->reader;
  delete data;

  jsCallback.Call({msg, reader->Value()});
}

void ReaderListener(pulsar_reader_t *cReader, pulsar_message_t *cMessage, void *ctx) {
  ReaderListenerCallback *readerListenerCallback = (ReaderListenerCallback *)ctx;
  Reader *reader = (Reader *)readerListenerCallback->reader;
  if (readerListenerCallback->callback.Acquire() != napi_ok) {
    return;
  }
  ReaderListenerProxyData *dataPtr = new ReaderListenerProxyData(cMessage, reader);
  readerListenerCallback->callback.BlockingCall(dataPtr, ReaderListenerProxy);
  readerListenerCallback->callback.Release();
}

void Reader::SetCReader(std::shared_ptr<CReaderWrapper> cReader) { this->wrapper = cReader; }
void Reader::SetListenerCallback(ReaderListenerCallback *listener) {
  if (listener) {
    // Maintain reference to reader, so it won't get garbage collected
    // since, when we have a listener, we don't have to maintain reference to reader (in js code)
    this->Ref();

    // Pass reader as argument
    listener->reader = this;
  }

  this->listener = listener;
}

Reader::Reader(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Reader>(info), listener(nullptr) {}

class ReaderNewInstanceWorker : public Napi::AsyncWorker {
 public:
  ReaderNewInstanceWorker(const Napi::Promise::Deferred &deferred, pulsar_client_t *cClient,
                          ReaderConfig *readerConfig, std::shared_ptr<CReaderWrapper> readerWrapper)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cClient(cClient),
        readerConfig(readerConfig),
        readerWrapper(readerWrapper),
        done(false) {}
  ~ReaderNewInstanceWorker() {}
  void Execute() {
    const std::string &topic = this->readerConfig->GetTopic();
    if (topic.empty()) {
      std::string msg("Topic is required and must be specified as a string when creating reader");
      SetError(msg);
      return;
    }
    if (this->readerConfig->GetCStartMessageId() == nullptr) {
      std::string msg(
          "StartMessageId is required and must be specified as a MessageId object when creating reader");
      SetError(msg);
      return;
    }

    pulsar_client_create_reader_async(this->cClient, topic.c_str(), this->readerConfig->GetCStartMessageId(),
                                      this->readerConfig->GetCReaderConfig(),
                                      &ReaderNewInstanceWorker::createReaderCallback, (void *)this);

    while (!done) {
      std::this_thread::yield();
    }
  }
  void OnOK() {
    Napi::Object obj = Reader::constructor.New({});
    Reader *reader = Reader::Unwrap(obj);
    reader->SetCReader(this->readerWrapper);
    reader->SetListenerCallback(this->listener);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_client_t *cClient;
  pulsar_reader_t *cReader;
  ReaderConfig *readerConfig;
  ReaderListenerCallback *listener;
  std::shared_ptr<CReaderWrapper> readerWrapper;
  std::atomic<bool> done;
  static void createReaderCallback(pulsar_result result, pulsar_reader_t *reader, void *ctx) {
    ReaderNewInstanceWorker *worker = (ReaderNewInstanceWorker *)ctx;
    if (result != pulsar_result_Ok) {
      worker->SetError(std::string("Failed to create reader: ") + pulsar_result_str(result));
    } else {
      worker->readerWrapper->cReader = reader;
      worker->listener = worker->readerConfig->GetListenerCallback();
    }

    delete worker->readerConfig;
    worker->done = true;
  }
};

Napi::Value Reader::NewInstance(const Napi::CallbackInfo &info, pulsar_client_t *cClient) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();

  std::shared_ptr<CReaderWrapper> readerWrapper = std::make_shared<CReaderWrapper>();

  ReaderConfig *readerConfig = new ReaderConfig(config, readerWrapper, &ReaderListener);
  ReaderNewInstanceWorker *wk = new ReaderNewInstanceWorker(deferred, cClient, readerConfig, readerWrapper);
  wk->Queue();
  return deferred.Promise();
}

class ReaderReadNextWorker : public Napi::AsyncWorker {
 public:
  ReaderReadNextWorker(const Napi::Promise::Deferred &deferred, pulsar_reader_t *cReader,
                       int64_t timeout = -1)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cReader(cReader),
        timeout(timeout) {}
  ~ReaderReadNextWorker() {}
  void Execute() {
    pulsar_result result;
    if (timeout > 0) {
      result = pulsar_reader_read_next_with_timeout(this->cReader, &(this->cMessage), timeout);
    } else {
      result = pulsar_reader_read_next(this->cReader, &(this->cMessage));
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
  pulsar_reader_t *cReader;
  pulsar_message_t *cMessage;
  int64_t timeout;
};

Napi::Value Reader::ReadNext(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  if (info[0].IsUndefined()) {
    ReaderReadNextWorker *wk = new ReaderReadNextWorker(deferred, this->wrapper->cReader);
    wk->Queue();
  } else {
    Napi::Number timeout = info[0].As<Napi::Object>().ToNumber();
    ReaderReadNextWorker *wk =
        new ReaderReadNextWorker(deferred, this->wrapper->cReader, timeout.Int64Value());
    wk->Queue();
  }
  return deferred.Promise();
}

Napi::Value Reader::HasNext(const Napi::CallbackInfo &info) {
  int value = 0;
  pulsar_result result = pulsar_reader_has_message_available(this->wrapper->cReader, &value);
  if (result != pulsar_result_Ok) {
    Napi::Error::New(info.Env(), "Failed to check if next message is available").ThrowAsJavaScriptException();
    return Napi::Boolean::New(info.Env(), false);
  } else if (value == 1) {
    return Napi::Boolean::New(info.Env(), true);
  } else {
    return Napi::Boolean::New(info.Env(), false);
  }
}

Napi::Value Reader::IsConnected(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, pulsar_reader_is_connected(this->wrapper->cReader));
}

class ReaderCloseWorker : public Napi::AsyncWorker {
 public:
  ReaderCloseWorker(const Napi::Promise::Deferred &deferred, pulsar_reader_t *cReader, Reader *reader)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cReader(cReader),
        reader(reader) {}
  ~ReaderCloseWorker() {}
  void Execute() {
    pulsar_result result = pulsar_reader_close(this->cReader);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }
  void OnOK() {
    this->reader->Cleanup();
    this->deferred.Resolve(Env().Null());
  }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to close reader: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_reader_t *cReader;
  Reader *reader;
};

Napi::Value Reader::Close(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ReaderCloseWorker *wk = new ReaderCloseWorker(deferred, this->wrapper->cReader, this);
  wk->Queue();
  return deferred.Promise();
}

void Reader::Cleanup() {
  if (this->listener) {
    this->CleanupListener();
  }
}

void Reader::CleanupListener() {
  this->Unref();
  this->listener->callback.Release();
  this->listener = nullptr;
}

Reader::~Reader() {
  if (this->listener) {
    this->CleanupListener();
  }
}
