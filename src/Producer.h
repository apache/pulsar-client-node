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

#ifndef PRODUCER_H
#define PRODUCER_H

#include <napi.h>
#include <pulsar/c/client.h>
#include <pulsar/c/producer.h>

class Producer : public Napi::ObjectWrap<Producer> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient);
  static Napi::FunctionReference constructor;
  Producer(const Napi::CallbackInfo &info);
  ~Producer();
  void SetCProducer(std::shared_ptr<pulsar_producer_t> cProducer);

 private:
  std::shared_ptr<pulsar_producer_t> cProducer;
  Napi::Value Send(const Napi::CallbackInfo &info);
  Napi::Value Flush(const Napi::CallbackInfo &info);
  Napi::Value Close(const Napi::CallbackInfo &info);
  Napi::Value GetProducerName(const Napi::CallbackInfo &info);
  Napi::Value GetTopic(const Napi::CallbackInfo &info);
  Napi::Value IsConnected(const Napi::CallbackInfo &info);
};

#endif
