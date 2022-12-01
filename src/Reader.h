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

#ifndef READER_H
#define READER_H

#include <napi.h>
#include <pulsar/c/client.h>
#include "ReaderConfig.h"

class Reader : public Napi::ObjectWrap<Reader> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient);
  static Napi::FunctionReference constructor;
  Reader(const Napi::CallbackInfo &info);
  ~Reader();
  void SetCReader(std::shared_ptr<pulsar_reader_t> cReader);
  void SetListenerCallback(ReaderListenerCallback *listener);
  void Cleanup();

 private:
  std::shared_ptr<pulsar_reader_t> cReader;
  ReaderListenerCallback *listener;

  Napi::Value ReadNext(const Napi::CallbackInfo &info);
  Napi::Value HasNext(const Napi::CallbackInfo &info);
  Napi::Value IsConnected(const Napi::CallbackInfo &info);
  Napi::Value Seek(const Napi::CallbackInfo &info);
  Napi::Value SeekTimestamp(const Napi::CallbackInfo &info);
  Napi::Value Close(const Napi::CallbackInfo &info);
  void CleanupListener();
};

#endif
