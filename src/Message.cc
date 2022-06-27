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
#include "MessageId.h"
#include <pulsar/c/message.h>

static const std::string CFG_DATA = "data";
static const std::string CFG_PROPS = "properties";
static const std::string CFG_EVENT_TIME = "eventTimestamp";
static const std::string CFG_SEQUENCE_ID = "sequenceId";
static const std::string CFG_PARTITION_KEY = "partitionKey";
static const std::string CFG_REPL_CLUSTERS = "replicationClusters";
static const std::string CFG_DELIVER_AFTER = "deliverAfter";
static const std::string CFG_DELIVER_AT = "deliverAt";
static const std::string CFG_DISABLE_REPLICATION = "disableReplication";
static const std::string CFG_ORDERING_KEY = "orderingKey";

Napi::FunctionReference Message::constructor;

Napi::Object Message::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "Message",
      {InstanceMethod("getTopicName", &Message::GetTopicName),
       InstanceMethod("getProperties", &Message::GetProperties), InstanceMethod("getData", &Message::GetData),
       InstanceMethod("getMessageId", &Message::GetMessageId),
       InstanceMethod("getPublishTimestamp", &Message::GetPublishTimestamp),
       InstanceMethod("getEventTimestamp", &Message::GetEventTimestamp),
       InstanceMethod("getRedeliveryCount", &Message::GetRedeliveryCount),
       InstanceMethod("getPartitionKey", &Message::GetPartitionKey)});

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("Message", func);
  return exports;
}

Napi::Object Message::NewInstance(Napi::Value arg, std::shared_ptr<pulsar_message_t> cMessage) {
  Napi::Object obj = constructor.New({});
  Message *msg = Unwrap(obj);
  msg->cMessage = cMessage;
  return obj;
}

Message::Message(const Napi::CallbackInfo &info) : Napi::ObjectWrap<Message>(info), cMessage(nullptr) {}

std::shared_ptr<pulsar_message_t> Message::GetCMessage() { return this->cMessage; }

Napi::Value Message::GetTopicName(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return Napi::String::New(env, pulsar_message_get_topic_name(this->cMessage.get()));
}

Napi::Value Message::GetRedeliveryCount(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return Napi::Number::New(env, pulsar_message_get_redelivery_count(this->cMessage.get()));
}

Napi::Value Message::GetProperties(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  Napi::Array arr = Napi::Array::New(env);
  pulsar_string_map_t *cProperties = pulsar_message_get_properties(this->cMessage.get());
  int size = pulsar_string_map_size(cProperties);
  for (int i = 0; i < size; i++) {
    arr.Set(pulsar_string_map_get_key(cProperties, i), pulsar_string_map_get_value(cProperties, i));
  }
  pulsar_string_map_free(cProperties);
  return arr;
}

Napi::Value Message::GetData(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  void *data = const_cast<void *>(pulsar_message_get_data(this->cMessage.get()));
  size_t size = (size_t)pulsar_message_get_length(this->cMessage.get());
  return Napi::Buffer<char>::Copy(env, (char *)data, size);
}

Napi::Value Message::GetMessageId(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return MessageId::NewInstanceFromMessage(info, this->cMessage);
}

Napi::Value Message::GetEventTimestamp(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return Napi::Number::New(env, pulsar_message_get_event_timestamp(this->cMessage.get()));
}

Napi::Value Message::GetPublishTimestamp(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return Napi::Number::New(env, pulsar_message_get_publish_timestamp(this->cMessage.get()));
}

Napi::Value Message::GetPartitionKey(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (!ValidateCMessage(env)) {
    return env.Null();
  }
  return Napi::String::New(env, pulsar_message_get_partitionKey(this->cMessage.get()));
}

bool Message::ValidateCMessage(Napi::Env env) {
  if (this->cMessage.get()) {
    return true;
  } else {
    Napi::Error::New(env, "Message has not been built").ThrowAsJavaScriptException();
    return false;
  }
}

std::shared_ptr<pulsar_message_t> Message::BuildMessage(Napi::Object conf) {
  std::shared_ptr<pulsar_message_t> cMessage(pulsar_message_create(), pulsar_message_free);

  if (conf.Has(CFG_DATA) && conf.Get(CFG_DATA).IsBuffer()) {
    Napi::Buffer<char> buf = conf.Get(CFG_DATA).As<Napi::Buffer<char>>();
    char *data = buf.Data();
    pulsar_message_set_content(cMessage.get(), data, buf.Length());
  }

  if (conf.Has(CFG_PROPS) && conf.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = conf.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      Napi::String key = arr.Get(i).ToString();
      Napi::String value = propObj.Get(key).ToString();
      pulsar_message_set_property(cMessage.get(), key.Utf8Value().c_str(), value.Utf8Value().c_str());
    }
  }

  if (conf.Has(CFG_EVENT_TIME) && conf.Get(CFG_EVENT_TIME).IsNumber()) {
    int64_t eventTimestamp = conf.Get(CFG_EVENT_TIME).ToNumber().Int64Value();
    if (eventTimestamp >= 0) {
      pulsar_message_set_event_timestamp(cMessage.get(), eventTimestamp);
    }
  }

  if (conf.Has(CFG_SEQUENCE_ID) && conf.Get(CFG_SEQUENCE_ID).IsNumber()) {
    Napi::Number sequenceId = conf.Get(CFG_SEQUENCE_ID).ToNumber();
    pulsar_message_set_sequence_id(cMessage.get(), sequenceId.Int64Value());
  }

  if (conf.Has(CFG_PARTITION_KEY) && conf.Get(CFG_PARTITION_KEY).IsString()) {
    Napi::String partitionKey = conf.Get(CFG_PARTITION_KEY).ToString();
    pulsar_message_set_partition_key(cMessage.get(), partitionKey.Utf8Value().c_str());
  }

  if (conf.Has(CFG_REPL_CLUSTERS) && conf.Get(CFG_REPL_CLUSTERS).IsArray()) {
    Napi::Array clusters = conf.Get(CFG_REPL_CLUSTERS).As<Napi::Array>();
    // Empty list means to disable replication
    int length = clusters.Length();
    if (length == 0) {
      pulsar_message_disable_replication(cMessage.get(), 1);
    } else {
      char **arr = NewStringArray(length);
      for (int i = 0; i < length; i++) {
        SetString(arr, clusters.Get(i).ToString().Utf8Value().c_str(), i);
      }
      pulsar_message_set_replication_clusters(cMessage.get(), (const char **)arr, length);
      FreeStringArray(arr, length);
    }
  }

  if (conf.Has(CFG_DELIVER_AFTER) && conf.Get(CFG_DELIVER_AFTER).IsNumber()) {
    Napi::Number deliverAfter = conf.Get(CFG_DELIVER_AFTER).ToNumber();
    pulsar_message_set_deliver_after(cMessage.get(), deliverAfter.Int64Value());
  }

  if (conf.Has(CFG_DELIVER_AT) && conf.Get(CFG_DELIVER_AT).IsNumber()) {
    Napi::Number deliverAt = conf.Get(CFG_DELIVER_AT).ToNumber();
    pulsar_message_set_deliver_at(cMessage.get(), deliverAt.Int64Value());
  }

  if (conf.Has(CFG_DISABLE_REPLICATION) && conf.Get(CFG_DISABLE_REPLICATION).IsBoolean()) {
    Napi::Boolean disableReplication = conf.Get(CFG_DISABLE_REPLICATION).ToBoolean();
    if (disableReplication.Value()) {
      pulsar_message_disable_replication(cMessage.get(), 1);
    }
  }

  if (conf.Has(CFG_ORDERING_KEY) && conf.Get(CFG_ORDERING_KEY).IsString()) {
    Napi::String orderingKey = conf.Get(CFG_ORDERING_KEY).ToString();
    pulsar_message_set_ordering_key(cMessage.get(), orderingKey.Utf8Value().c_str());
  }

  return cMessage;
}

Message::~Message() {}
