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

#ifndef MESSAGE_ID_H
#define MESSAGE_ID_H

#include <napi.h>
#include <pulsar/c/message.h>
#include <pulsar/c/message_id.h>

class MessageId : public Napi::ObjectWrap<MessageId> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance(Napi::Value arg);
  static Napi::Object NewInstance(std::shared_ptr<pulsar_message_id_t> cMessageId);
  static Napi::Object NewInstanceFromMessage(const Napi::CallbackInfo &info,
                                             std::shared_ptr<pulsar_message_t> cMessage);
  static Napi::Value Earliest(const Napi::CallbackInfo &info);
  static Napi::Value Latest(const Napi::CallbackInfo &info);
  Napi::Value Serialize(const Napi::CallbackInfo &info);
  static Napi::Value Deserialize(const Napi::CallbackInfo &info);
  MessageId(const Napi::CallbackInfo &info);
  ~MessageId();
  std::shared_ptr<pulsar_message_id_t> GetCMessageId();

 private:
  static Napi::FunctionReference constructor;
  std::shared_ptr<pulsar_message_id_t> cMessageId;

  Napi::Value ToString(const Napi::CallbackInfo &info);
};

#endif
