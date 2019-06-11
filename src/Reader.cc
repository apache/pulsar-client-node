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

Napi::FunctionReference Reader::constructor;

void Reader::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "Reader",
                                    {
                                        InstanceMethod("readNext", &Reader::ReadNext),
                                        InstanceMethod("hasNext", &Reader::HasNext),
                                        InstanceMethod("close", &Reader::Close),
                                    });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();
}

void Reader::SetCReader(pulsar_reader_t *cReader) { this->cReader = cReader; }

Reader::Reader(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Reader>(info) {}

class ReaderNewInstanceWorker : public Napi::AsyncWorker {
 public:
  ReaderNewInstanceWorker(const Napi::Promise::Deferred &deferred, pulsar_client_t *cClient,
                          ReaderConfig *readerConfig)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cClient(cClient),
        readerConfig(readerConfig) {}
  ~ReaderNewInstanceWorker() {}
  void Execute() {
    const std::string &topic = this->readerConfig->GetTopic();
    if (topic.empty()) {
      SetError(std::string("Topic is required and must be specified as a string when creating reader"));
      return;
    }
    if (this->readerConfig->GetCStartMessageId() == nullptr) {
      SetError(std::string(
          "StartMessageId is required and must be specified as a MessageId object when creating reader"));
      return;
    }

    pulsar_result result =
        pulsar_client_create_reader(this->cClient, topic.c_str(), this->readerConfig->GetCStartMessageId(),
                                    this->readerConfig->GetCReaderConfig(), &(this->cReader));
    delete this->readerConfig;
    if (result != pulsar_result_Ok) {
      SetError(std::string("Failed to create reader: ") + pulsar_result_str(result));
      return;
    }
  }
  void OnOK() {
    Napi::Object obj = Reader::constructor.New({});
    Reader *reader = Reader::Unwrap(obj);
    reader->SetCReader(this->cReader);
    this->deferred.Resolve(obj);
  }
  void OnError(const Napi::Error &e) { this->deferred.Reject(Napi::Error::New(Env(), e.Message()).Value()); }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_client_t *cClient;
  ReaderConfig *readerConfig;
  pulsar_reader_t *cReader;
};

Napi::Value Reader::NewInstance(const Napi::CallbackInfo &info, pulsar_client_t *cClient) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  Napi::Object config = info[0].As<Napi::Object>();
  ReaderConfig *readerConfig = new ReaderConfig(config);
  ReaderNewInstanceWorker *wk = new ReaderNewInstanceWorker(deferred, cClient, readerConfig);
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
  pulsar_result result = pulsar_reader_has_message_available(this->cReader, &value);
  if (result != pulsar_result_Ok) {
    Napi::Error::New(info.Env(), "Failed to check if next message is available").ThrowAsJavaScriptException();
    return Napi::Boolean::New(info.Env(), false);
  } else if (value == 1) {
    return Napi::Boolean::New(info.Env(), true);
  } else {
    return Napi::Boolean::New(info.Env(), false);
  }
}

class ReaderCloseWorker : public Napi::AsyncWorker {
 public:
  ReaderCloseWorker(const Napi::Promise::Deferred &deferred, pulsar_reader_t *cReader)
      : AsyncWorker(Napi::Function::New(deferred.Promise().Env(), [](const Napi::CallbackInfo &info) {})),
        deferred(deferred),
        cReader(cReader) {}
  ~ReaderCloseWorker() {}
  void Execute() {
    pulsar_result result = pulsar_reader_close(this->cReader);
    if (result != pulsar_result_Ok) SetError(pulsar_result_str(result));
  }
  void OnOK() { this->deferred.Resolve(Env().Null()); }
  void OnError(const Napi::Error &e) {
    this->deferred.Reject(
        Napi::Error::New(Env(), std::string("Failed to close reader: ") + e.Message()).Value());
  }

 private:
  Napi::Promise::Deferred deferred;
  pulsar_reader_t *cReader;
};

Napi::Value Reader::Close(const Napi::CallbackInfo &info) {
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
  ReaderCloseWorker *wk = new ReaderCloseWorker(deferred, this->cReader);
  wk->Queue();
  return deferred.Promise();
}

Reader::~Reader() { pulsar_reader_free(this->cReader); }
