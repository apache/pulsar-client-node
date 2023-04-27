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
#include "ConsumerConfig.h"
#include "MessageListener.h"

class Consumer : public Napi::ObjectWrap<Consumer> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Value NewInstance(const Napi::CallbackInfo &info, std::shared_ptr<pulsar_client_t> cClient);
  static Napi::FunctionReference constructor;
  Consumer(const Napi::CallbackInfo &info);
  ~Consumer();
  void SetCConsumer(std::shared_ptr<pulsar_consumer_t> cConsumer);
  void SetListenerCallback(MessageListenerCallback *listener);
  void Cleanup();
  void CleanupListener();
  std::string GetTopic();
  std::string GetSubscriptionName();

 private:
  std::shared_ptr<pulsar_consumer_t> cConsumer;
  MessageListenerCallback *listener;

  Napi::Value Receive(const Napi::CallbackInfo &info);
  Napi::Value Acknowledge(const Napi::CallbackInfo &info);
  Napi::Value AcknowledgeId(const Napi::CallbackInfo &info);
  void NegativeAcknowledge(const Napi::CallbackInfo &info);
  void NegativeAcknowledgeId(const Napi::CallbackInfo &info);
  Napi::Value AcknowledgeCumulative(const Napi::CallbackInfo &info);
  Napi::Value AcknowledgeCumulativeId(const Napi::CallbackInfo &info);
  Napi::Value Seek(const Napi::CallbackInfo &info);
  Napi::Value SeekTimestamp(const Napi::CallbackInfo &info);
  Napi::Value IsConnected(const Napi::CallbackInfo &info);
  Napi::Value Close(const Napi::CallbackInfo &info);
  Napi::Value Unsubscribe(const Napi::CallbackInfo &info);
};

#endif
