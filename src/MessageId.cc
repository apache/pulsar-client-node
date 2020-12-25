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

#include "MessageId.h"
#include <pulsar/c/message.h>
#include <pulsar/c/message_id.h>

Napi::FunctionReference MessageId::constructor;

Napi::Object MessageId::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "MessageId",
                                    {StaticMethod("earliest", &MessageId::Earliest, napi_static),
                                     StaticMethod("latest", &MessageId::Latest, napi_static),
                                     StaticMethod("finalize", &MessageId::Free, napi_static),
                                     InstanceMethod("serialize", &MessageId::Serialize),
                                     StaticMethod("deserialize", &MessageId::Deserialize, napi_static),
                                     InstanceMethod("toString", &MessageId::ToString)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("MessageId", func);
  return exports;
}

MessageId::MessageId(const Napi::CallbackInfo &info) : Napi::ObjectWrap<MessageId>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
}

Napi::Object MessageId::NewInstanceFromMessage(const Napi::CallbackInfo &info, pulsar_message_t *cMessage) {
  Napi::Object obj = NewInstance(info[0]);
  MessageId *msgId = Unwrap(obj);
  msgId->cMessageId = pulsar_message_get_message_id(cMessage);
  return obj;
}

Napi::Object MessageId::NewInstance(Napi::Value arg) {
  Napi::Object obj = constructor.New({arg});
  return obj;
}

Napi::Object MessageId::NewInstance(pulsar_message_id_t *cMessageId) {
  Napi::Object obj = constructor.New({});
  MessageId *msgId = Unwrap(obj);
  msgId->cMessageId = cMessageId;
  return obj;
}

void MessageId::Free(const Napi::CallbackInfo &info) {
  Napi::Object obj = info[0].As<Napi::Object>();
  MessageId *msgId = Unwrap(obj);
  pulsar_message_id_free(msgId->cMessageId);
}

Napi::Value MessageId::Earliest(const Napi::CallbackInfo &info) {
  Napi::Object obj = NewInstance(info[0]);
  MessageId *msgId = Unwrap(obj);
  msgId->cMessageId = (pulsar_message_id_t *)pulsar_message_id_earliest();
  msgId->skipCMessageIdFree = true;
  return obj;
}

Napi::Value MessageId::Latest(const Napi::CallbackInfo &info) {
  Napi::Object obj = NewInstance(info[0]);
  MessageId *msgId = Unwrap(obj);
  msgId->cMessageId = (pulsar_message_id_t *)pulsar_message_id_latest();
  msgId->skipCMessageIdFree = true;
  return obj;
}

Napi::Value MessageId::Serialize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  int len;
  void *ptr = pulsar_message_id_serialize(GetCMessageId(), &len);

  return Napi::Buffer<char>::New(env, (char *)ptr, len);
}

Napi::Value MessageId::Deserialize(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!info[0].IsBuffer()) {
    Napi::Error::New(env, "Expected buffer as first argument").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Object obj = NewInstance(info[0]);
  MessageId *msgId = Unwrap(obj);

  Napi::Buffer<char> buf = info[0].As<Napi::Buffer<char>>();
  char *data = buf.Data();
  msgId->cMessageId = (pulsar_message_id_t *)pulsar_message_id_deserialize(data, buf.Length());

  return obj;
}

pulsar_message_id_t *MessageId::GetCMessageId() { return this->cMessageId; }

Napi::Value MessageId::ToString(const Napi::CallbackInfo &info) {
  return Napi::String::New(info.Env(), pulsar_message_id_str(this->cMessageId));
}

MessageId::~MessageId() {
  if (!this->skipCMessageIdFree) {
    pulsar_message_id_free(this->cMessageId);
  }
}
