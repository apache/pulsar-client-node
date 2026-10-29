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
#include "SchemaInfo.h"
#include <pulsar/ConsumerConfiguration.h>
#include <pulsar/ProducerConfiguration.h>
#include <map>

static const std::string CFG_SCHEMA_TYPE = "schemaType";
static const std::string CFG_NAME = "name";
static const std::string CFG_SCHEMA = "schema";
static const std::string CFG_PROPS = "properties";

struct _pulsar_producer_configuration {
  pulsar::ProducerConfiguration conf;
};

struct _pulsar_consumer_configuration {
  pulsar::ConsumerConfiguration consumerConfiguration;
};

static const std::map<std::string, pulsar::SchemaType> SCHEMA_TYPE = {
    {"None", static_cast<pulsar::SchemaType>(0)},
    {"String", static_cast<pulsar::SchemaType>(1)},
    {"Json", static_cast<pulsar::SchemaType>(2)},
    {"Protobuf", static_cast<pulsar::SchemaType>(3)},
    {"Avro", static_cast<pulsar::SchemaType>(4)},
    {"Boolean", static_cast<pulsar::SchemaType>(5)},
    {"Int8", static_cast<pulsar::SchemaType>(6)},
    {"Int16", static_cast<pulsar::SchemaType>(7)},
    {"Int32", static_cast<pulsar::SchemaType>(8)},
    {"Int64", static_cast<pulsar::SchemaType>(9)},
    {"Float32", static_cast<pulsar::SchemaType>(10)},
    {"Float64", static_cast<pulsar::SchemaType>(11)},
    {"KeyValue", static_cast<pulsar::SchemaType>(15)},
    {"ProtobufNative", static_cast<pulsar::SchemaType>(20)},
    {"Bytes", static_cast<pulsar::SchemaType>(-1)},
    {"AutoConsume", static_cast<pulsar::SchemaType>(-3)},
    {"AutoPublish", static_cast<pulsar::SchemaType>(-4)}};

SchemaInfo::SchemaInfo(const Napi::Object &schemaInfo)
    : schemaType(static_cast<pulsar::SchemaType>(-1)), name("BYTES"), schema() {
  if (schemaInfo.Has(CFG_SCHEMA_TYPE) && schemaInfo.Get(CFG_SCHEMA_TYPE).IsString()) {
    std::string typeStr = schemaInfo.Get(CFG_SCHEMA_TYPE).ToString().Utf8Value();
    auto it = SCHEMA_TYPE.find(typeStr);
    if (it == SCHEMA_TYPE.end()) {
      Napi::TypeError::New(schemaInfo.Env(), "Unknown schemaType: " + typeStr).ThrowAsJavaScriptException();
      return;
    }
    this->name = typeStr;
    this->schemaType = it->second;
  }
  if (schemaInfo.Has(CFG_NAME) && schemaInfo.Get(CFG_NAME).IsString()) {
    this->name = schemaInfo.Get(CFG_NAME).ToString().Utf8Value();
  }
  if (schemaInfo.Has(CFG_SCHEMA) && schemaInfo.Get(CFG_SCHEMA).IsString()) {
    this->schema = schemaInfo.Get(CFG_SCHEMA).ToString().Utf8Value();
  } else if (schemaInfo.Has(CFG_SCHEMA) && schemaInfo.Get(CFG_SCHEMA).IsBuffer()) {
    Napi::Buffer<char> buffer = schemaInfo.Get(CFG_SCHEMA).As<Napi::Buffer<char>>();
    this->schema = std::string(buffer.Data(), buffer.Length());
  }
  if (schemaInfo.Has(CFG_PROPS) && schemaInfo.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = schemaInfo.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      Napi::String key = arr.Get(i).ToString();
      Napi::String value = propObj.Get(key).ToString();
      this->properties[key.Utf8Value()] = value.Utf8Value();
    }
  }
}

void SchemaInfo::SetProducerSchema(std::shared_ptr<pulsar_producer_configuration_t> cProducerConfiguration) {
  cProducerConfiguration->conf.setSchema(
      pulsar::SchemaInfo(this->schemaType, this->name, this->schema, this->properties));
}

void SchemaInfo::SetConsumerSchema(std::shared_ptr<pulsar_consumer_configuration_t> cConsumerConfiguration) {
  cConsumerConfiguration->consumerConfiguration.setSchema(
      pulsar::SchemaInfo(this->schemaType, this->name, this->schema, this->properties));
}

std::optional<std::string> SchemaInfo::GetSchemaTypeName(pulsar::SchemaType schemaType) {
  for (const auto &entry : SCHEMA_TYPE) {
    if (entry.second == schemaType) {
      return entry.first;
    }
  }
  return std::nullopt;
}

Napi::Object SchemaInfo::NewObject(Napi::Env env, const std::string &schemaTypeName,
                                   const pulsar::SchemaInfo &schemaInfo) {
  Napi::Object obj = Napi::Object::New(env);
  obj.Set(CFG_SCHEMA_TYPE, Napi::String::New(env, schemaTypeName));
  obj.Set(CFG_NAME, Napi::String::New(env, schemaInfo.getName()));
  // Schema data is bytes: KeyValue schemas, for one, are binary and would not survive a UTF-8 string.
  const std::string &schema = schemaInfo.getSchema();
  obj.Set(CFG_SCHEMA, Napi::Buffer<char>::Copy(env, schema.data(), schema.size()));
  Napi::Object props = Napi::Object::New(env);
  for (const auto &prop : schemaInfo.getProperties()) {
    // Define own properties: Set would route a "__proto__" key to the prototype setter and drop it.
    props.DefineProperty(Napi::PropertyDescriptor::Value(
        prop.first, Napi::String::New(env, prop.second),
        static_cast<napi_property_attributes>(napi_writable | napi_enumerable | napi_configurable)));
  }
  obj.Set(CFG_PROPS, props);
  return obj;
}

SchemaInfo::~SchemaInfo() {}
