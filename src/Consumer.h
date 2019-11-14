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

#ifndef CONSUMER_H
#define CONSUMER_H

#include <napi.h>
#include <pulsar/c/client.h>

class Consumer : public Napi::ObjectWrap<Consumer> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(const Napi::CallbackInfo &info, pulsar_client_t *cClient);
  static Napi::FunctionReference constructor;
  Consumer(const Napi::CallbackInfo &info);
  ~Consumer();
  void SetCConsumer(pulsar_consumer_t *cConsumer);

 private:
  pulsar_consumer_t *cConsumer;

  Napi::Value Receive(const Napi::CallbackInfo &info);
  void Acknowledge(const Napi::CallbackInfo &info);
  void AcknowledgeId(const Napi::CallbackInfo &info);
  void NegativeAcknowledge(const Napi::CallbackInfo &info);
  void NegativeAcknowledgeId(const Napi::CallbackInfo &info);
  void AcknowledgeCumulative(const Napi::CallbackInfo &info);
  void AcknowledgeCumulativeId(const Napi::CallbackInfo &info);
  Napi::Value Close(const Napi::CallbackInfo &info);
};

#endif
